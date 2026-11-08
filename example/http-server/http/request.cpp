#include "request.h"

#include <sstream>

namespace fd::example::http {

std::string request::handle(std::string_view request) noexcept {
    if (!parse(request)) {
        return std::string{"HTTP/1.1 400 NOK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 0\r\n"
            "\r\n"};
    }

    return std::string{"HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 52\r\n"
        "\r\n"
        "Hello from HTTP server written using the FD library!"};
}

bool request::parse(std::string_view request) noexcept {
    std::istringstream iss{std::string(request)};

    iss >> method_;
    iss >> path_;
    iss >> version_;

    return iss.good();
}
}