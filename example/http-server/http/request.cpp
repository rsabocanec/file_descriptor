#include "request.h"

#include <fstream>
#include <sstream>

namespace fd::example::http {

bool request::parse(std::string_view request) noexcept {
    std::istringstream iss{std::string(request)};

    iss >> method_;
    iss >> path_;
    iss >> version_;

    if (!iss.good()) {
        logger_->error("Failed to parse request: '{}'", request);
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
            logger_->error("Failed to parse header: '{}'", header_line);
            return false;
        }

        header_.emplace_back(header_line.substr(0, colon_pos), header_line.substr(colon_pos + 1));
        logger_->info("Parsed header: '{}'", header_line);
    }

    iss >> body_;

    logger_->info("Parsed request body '{}'", body_);

    return iss.good() || iss.eof();
}

std::string request::handle_get() const noexcept {

    std::string path{path_};

    if (path == "/") {
        path = "/index.html";
    }

    if (path_.contains("..")) {
        return handle_error(400);
    }

    std::fstream ofs{"www/" +path, std::ios::in | std::ios::binary};

    if (!ofs) {
        return handle_error(404);
    }

    std::string content{std::istreambuf_iterator<char>{ofs}, std::istreambuf_iterator<char>{}};

    std::string response{"HTTP/1.1 200 OK\r\n"
                         "Content-Type: text/html\r\n"
                         "Content-Length: " + std::to_string(content.size()) + "\r\n"
                         "\r\n"
                         + content};

    return response;
}

std::string request::handle_post() const noexcept {
    return {};
}

std::string request::handle_put() const noexcept {
    return {};
}

std::string request::handle_delete() const noexcept {
    return {};
}

std::string request::handle_head() const noexcept {
    return {};
}

std::string request::handle_options() const noexcept {
    return {};
}

std::string request::handle_patch() const noexcept {
    return {};
}

std::string request::handle_trace() const noexcept {
    return {};
}

std::string request::handle_error(uint16_t status_code) noexcept {
    std::ostringstream oss{};

    oss << "HTTP/1.1 " << status_code << " NOK\r\n"
        << "Content-Type: text/plain\r\n"
        << "Content-Length: 0\r\n"
        << "\r\n";

    return oss.str();
}
}