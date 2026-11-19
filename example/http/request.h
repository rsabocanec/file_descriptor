#ifndef FD_EXAMPLE_HTTP_REQUEST_H
#define FD_EXAMPLE_HTTP_REQUEST_H

#pragma once

#include <descriptor.h>
#include <stream_socket.h>

#include <string>
#include <vector>
#include <memory>
#include <functional>

#include <cstdint>

#include <spdlog/spdlog.h>


namespace fd::example::http {

using header = std::vector<std::pair<std::string, std::string>>;

using request_handler = std::function<fd::expected_result(  std::string_view,   // method
                                                            std::string_view,   // path
                                                            std::string_view,   // version
                                                            const header&,      // headers
                                                            std::string_view,   // body
                                                            const fd::acceptor&,
                                                            std::shared_ptr<spdlog::logger>)>;

constexpr auto default_request_handler = [](std::string_view method,
                                            std::string_view path,
                                            std::string_view/* version*/,
                                            const header&/* headers*/,
                                            std::string_view/* body*/,
                                            const fd::acceptor& responder,
                                            std::shared_ptr<spdlog::logger> logger) -> fd::expected_result {

    logger->info("Default '{}' request handler called for path '{}'", method, path);

    constexpr std::string_view response{"HTTP/1.1 501 Not Implemented\r\n"
                                        "Content-Type: text/plain\r\n"
                                        "Content-Length: 0\r\n"
                                        "\r\n"};

    if (auto result = responder.write(response); !result) {
        logger->error("Failed to send response with result {} '{}'",
                        result.error().value(), result.error().message());
        return result;
    }

    return {};
};

class request {
    std::string method_{};
    std::string path_{};
    std::string version_{};

    header header_{};
    std::string body_{};

    request_handler get_handler_{default_request_handler};
    request_handler post_handler_{default_request_handler};
    request_handler put_handler_{default_request_handler};
    request_handler delete_handler_{default_request_handler};
    request_handler head_handler_{default_request_handler};
    request_handler options_handler_{default_request_handler};
    request_handler patch_handler_{default_request_handler};
    request_handler trace_handler_{default_request_handler};

public:
    request() = default;

    request(const request&) = delete;
    request(request&&) = default;

    request& operator=(const request&) = delete;
    request& operator=(request&&) = default;

    ~request() = default;

    [[nodiscard]] fd::expected_result handle(std::string_view request,
                                             const fd::acceptor& acceptor,
                                             std::shared_ptr<spdlog::logger> logger) noexcept;

    void set_get_handler(request_handler handler) noexcept {
        get_handler_ = std::move(handler);
    }

    void set_post_handler(request_handler handler) noexcept {
        post_handler_ = std::move(handler);
    }

    void set_put_handler(request_handler handler) noexcept {
        put_handler_ = std::move(handler);
    }

    void set_delete_handler(request_handler handler) noexcept {
        delete_handler_ = std::move(handler);
    }

    void set_head_handler(request_handler handler) noexcept {
        head_handler_ = std::move(handler);
    }

    void set_options_handler(request_handler handler) noexcept {
        options_handler_ = std::move(handler);
    }

    void set_patch_handler(request_handler handler) noexcept {
        patch_handler_ = std::move(handler);
    }

    void set_trace_handler(request_handler handler) noexcept {
        trace_handler_ = std::move(handler);
    }

    [[nodiscard]] static fd::expected_result handle_error(uint16_t status_code,
                                                          const fd::acceptor& acceptor) noexcept;

private:
    [[nodiscard]] bool parse(std::string_view request,
                             std::shared_ptr<spdlog::logger> logger) noexcept;
};
} // namespace fd::example::http

#endif // FD_EXAMPLE_HTTP_REQUEST_H