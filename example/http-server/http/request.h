#ifndef FD_EXAMPLE_HTTP_REQUEST_H
#define FD_EXAMPLE_HTTP_REQUEST_H

#pragma once

#include <descriptor.h>
#include <stream_socket.h>

#include <string>
#include <vector>
#include <memory>

#include <cstdint>

#include <spdlog/spdlog.h>


namespace fd::example::http {

using header = std::vector<std::pair<std::string, std::string>>;

class request {
    std::string method_{};
    std::string path_{};
    std::string version_{};

    header header_{};
    std::string body_{};

    std::shared_ptr<spdlog::logger> logger_{};

public:
    explicit request(std::shared_ptr<spdlog::logger> logger)
    : logger_(logger) {}

    request(const request&) = delete;
    request(request&&) = default;

    request& operator=(const request&) = delete;
    request& operator=(request&&) = default;

    ~request() = default;

    [[nodiscard]] fd::expected_result handle(std::string_view request,
                                             const fd::acceptor& acceptor) noexcept  {
        if (!parse(request)) {
            return handle_error(400, acceptor);
        }

        if (method_ == "GET") {
            return handle_get(acceptor);
        }
        else if (method_ == "POST") {
            return handle_post(acceptor);
        }
        else if (method_ == "PUT") {
            return handle_put(acceptor);
        }
        else if (method_ == "DELETE") {
            return handle_delete(acceptor);
        }
        else if (method_ == "HEAD") {
            return handle_head(acceptor);
        }
        else if (method_ == "OPTIONS") {
            return handle_options(acceptor);
        }
        else if (method_ == "PATCH") {
            return handle_patch(acceptor);
        }
        else if (method_ == "TRACE") {
            return handle_trace(acceptor);
        }
        else {
            return handle_error(501, acceptor);
        }
    }

private:
    [[nodiscard]] bool parse(std::string_view request) noexcept;

    [[nodiscard]] fd::expected_result handle_get(const fd::acceptor& acceptor) const noexcept;
    [[nodiscard]] fd::expected_result handle_post(const fd::acceptor& acceptor) const noexcept;
    [[nodiscard]] fd::expected_result handle_put(const fd::acceptor& acceptor) const noexcept;
    [[nodiscard]] fd::expected_result handle_delete(const fd::acceptor& acceptor) const noexcept;
    [[nodiscard]] fd::expected_result handle_head(const fd::acceptor& acceptor) const noexcept;
    [[nodiscard]] fd::expected_result handle_options(const fd::acceptor& acceptor) const noexcept;
    [[nodiscard]] fd::expected_result handle_patch(const fd::acceptor& acceptor) const noexcept;
    [[nodiscard]] fd::expected_result handle_trace(const fd::acceptor& acceptor) const noexcept;

    [[nodiscard]] static fd::expected_result handle_error(uint16_t status_code, const fd::acceptor& acceptor) noexcept;
};
} // namespace fd::example::http

#endif // FD_EXAMPLE_HTTP_REQUEST_H