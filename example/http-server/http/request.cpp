#include "request.h"

#include <file.h>

#include <fstream>
#include <sstream>
#include <filesystem>

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

fd::expected_result request::handle_get(const fd::acceptor& acceptor) const noexcept {

    std::string path{path_};

    if (path == "/") {
        path = "/index.html";
    }

    if (path_.contains("..")) {
        return handle_error(400, acceptor);
    }

    std::string full_path{"www/"};
    full_path += path;

    if (!std::filesystem::exists(full_path)) {
        return handle_error(404, acceptor);
    }

    if (std::filesystem::is_directory(full_path)) {
        full_path += "/index.html";
    }

    fd::file file{};

    if (auto result = file.open(full_path, fd::open_flags::read_only); result) {
        logger_->error("Failed to open file {} with result {} '{}'",
                        full_path, result.value(), result.message());
        return handle_error(404, acceptor);
    }

    std::string response{"HTTP/1.1 200 OK\r\n"
                         "Content-Type: text/html\r\n"
                         "Content-Length: " + std::to_string(file.size()) + "\r\n"
                         "\r\n"};

    if (auto result = acceptor.write(response); !result) {
        logger_->error("Failed to send response header with result {} '{}'",
                        result.error().value(), result.error().message());
        return handle_error(500, acceptor);
    }

    return acceptor.splice(file, file.size());
}

fd::expected_result request::handle_post(const fd::acceptor& acceptor) const noexcept {
    return {};
}

fd::expected_result request::handle_put(const fd::acceptor& acceptor) const noexcept {
    return {};
}

fd::expected_result request::handle_delete(const fd::acceptor& acceptor) const noexcept {
    return {};
}

fd::expected_result request::handle_head(const fd::acceptor& acceptor) const noexcept {
    return {};
}

fd::expected_result request::handle_options(const fd::acceptor& acceptor) const noexcept {
    return {};
}

fd::expected_result request::handle_patch(const fd::acceptor& acceptor) const noexcept {
    return {};
}

fd::expected_result request::handle_trace(const fd::acceptor& acceptor) const noexcept {
    return {};
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