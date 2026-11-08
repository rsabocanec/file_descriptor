#ifndef FD_EXAMPLE_HTTP_REQUEST_H
#define FD_EXAMPLE_HTTP_REQUEST_H

#pragma once

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

    [[nodiscard]] std::string handle(std::string_view request) noexcept  {
        if (!parse(request)) {
            return handle_error(400);
        }

        if (method_ == "GET") {
            return handle_get();
        }
        else if (method_ == "POST") {
            return handle_post();
        }
        else if (method_ == "PUT") {
            return handle_put();
        }
        else if (method_ == "DELETE") {
            return handle_delete();
        }
        else if (method_ == "HEAD") {
            return handle_head();
        }
        else if (method_ == "OPTIONS") {
            return handle_options();
        }
        else if (method_ == "PATCH") {
            return handle_patch();
        }
        else if (method_ == "TRACE") {
            return handle_trace();
        }
        else {
            return handle_error(501);
        }
    }

private:
    [[nodiscard]] bool parse(std::string_view request) noexcept;

    [[nodiscard]] std::string handle_get() const noexcept;
    [[nodiscard]] std::string handle_post() const noexcept;
    [[nodiscard]] std::string handle_put() const noexcept;
    [[nodiscard]] std::string handle_delete() const noexcept;
    [[nodiscard]] std::string handle_head() const noexcept;
    [[nodiscard]] std::string handle_options() const noexcept;
    [[nodiscard]] std::string handle_patch() const noexcept;
    [[nodiscard]] std::string handle_trace() const noexcept;

    [[nodiscard]] static std::string handle_error(uint16_t status_code) noexcept;
};
} // namespace fd::example::http

#endif // FD_EXAMPLE_HTTP_REQUEST_H