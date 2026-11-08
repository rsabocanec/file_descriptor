#ifndef FD_EXAMPLE_HTTP_REQUEST_H
#define FD_EXAMPLE_HTTP_REQUEST_H

#pragma once

#include <string>
#include <vector>

namespace fd::example::http {

class request {
    std::string method_{};
    std::string path_{};
    std::string version_{};

    std::vector<std::pair<std::string, std::string>> headers_{};
    std::string body_{};

public:
    request() = default;

    [[nodiscard]] std::string handle(std::string_view request) noexcept;

private:
    [[nodiscard]] bool parse(std::string_view request) noexcept;
};
} // namespace fd::example::http

#endif // FD_EXAMPLE_HTTP_REQUEST_H