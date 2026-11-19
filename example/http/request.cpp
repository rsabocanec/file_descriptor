#include "request.h"

#include <file.h>

#include <fstream>
#include <sstream>
#include <filesystem>

namespace fd::example::http {

[[nodiscard]] fd::expected_result request::handle(std::string_view request,
                                                  const fd::acceptor& acceptor,
                                                  std::shared_ptr<spdlog::logger> logger) noexcept  {

    fd::expected_result result = {};

    if (!parse(request, logger)) {
        result = handle_error(400, acceptor);
    }

    if (method_ == "GET") {
        result = get_handler_(method_, path_, version_, header_, body_, acceptor, logger);
    }
    else if (method_ == "POST") {
        result = post_handler_(method_, path_, version_, header_, body_, acceptor, logger);
    }
    else if (method_ == "PUT") {
        result = put_handler_(method_, path_, version_, header_, body_, acceptor, logger);
    }
    else if (method_ == "DELETE") {
        result = delete_handler_(method_, path_, version_, header_, body_, acceptor, logger);
    }
    else if (method_ == "HEAD") {
        result = head_handler_(method_, path_, version_, header_, body_, acceptor, logger);
    }
    else if (method_ == "OPTIONS") {
        result = options_handler_(method_, path_, version_, header_, body_, acceptor, logger);
    }
    else if (method_ == "PATCH") {
        result = patch_handler_(method_, path_, version_, header_, body_, acceptor, logger);
    }
    else if (method_ == "TRACE") {
        result = trace_handler_(method_, path_, version_, header_, body_, acceptor, logger);
    }
    else {
        result = handle_error(501, acceptor);
    }

    return result;
}

bool request::parse(std::string_view request, std::shared_ptr<spdlog::logger> logger) noexcept {
    std::istringstream iss{std::string(request)};

    iss >> method_;
    iss >> path_;
    iss >> version_;

    if (!iss.good()) {
        logger->error("Failed to parse request: '{}'", request);
        return false;
    }

    std::string header_line{};

    while (std::getline(iss, header_line)) {
        if (header_line == "\r") {
            continue;
        }

        if (header_line.empty()) {
            break;
        }

        auto const colon_pos = header_line.find(':');
        if (colon_pos == std::string::npos) {
            logger->error("Failed to parse header: '{}'", header_line);
            return false;
        }

        header_.emplace_back(header_line.substr(0, colon_pos), header_line.substr(colon_pos + 1));
        logger->info("Parsed header: '{}'", header_line);
    }

    iss >> body_;

    logger->info("Parsed request body '{}'", body_);

    return iss.good() || iss.eof();
}

fd::expected_result request::handle_error(uint16_t status_code, const fd::acceptor& acceptor) noexcept {
    std::ostringstream oss{};

    oss << "HTTP/1.1 " << status_code << " NOK\r\n"
        << "Content-Type: text/plain\r\n"
        << "Content-Length: 0\r\n"
        << "\r\n";

    return acceptor.write(oss.str());
}
}