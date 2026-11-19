#include <tcp_socket.h>
#include <file.h>

#include "../http/request.h"
#include "../utility.hpp"

#include <array>
#include <span>
#include <atomic>
#include <csignal>

namespace {
    volatile std::sig_atomic_t signal_status;

    fd::tcp_socket *g_server{nullptr};

    constexpr std::string_view default_certificate_path{"~/.ssh/selfsigned"};
}

void signal_handler(int signal) {
    signal_status = signal;

    switch (signal) {
        case SIGTERM:
            if (g_server != nullptr) {
                [[maybe_unused]] auto const result = g_server->shutdown();
                g_server = nullptr;
            }
            break;
        default:
            break;
    }
}

namespace fd::example::http::handler {

fd::expected_result get(std::string_view/* method*/,
                        std::string_view path,
                        std::string_view/* version*/,
                        const header&/* headers*/,
                        std::string_view/* body*/,
                        const fd::acceptor& acceptor,
                        std::shared_ptr<spdlog::logger> logger) noexcept {

    std::string full_path{"www"};
    full_path += path;

    if (path == "/") {
        full_path += "index.html";
    }

    if (path.contains("..")) {
        return request::handle_error(403, acceptor);
    }

    if (!std::filesystem::exists(full_path)) {
        return request::handle_error(404, acceptor);
    }

    if (std::filesystem::is_directory(full_path)) {
        if (!full_path.ends_with('/')) {
            full_path += '/';
        }

        full_path += "index.html";
        if (!std::filesystem::exists(full_path)) {
            return request::handle_error(404, acceptor);
        }
    }

    fd::file file{};

    if (auto result = file.open(full_path, fd::open_flags::read_only); result) {
        logger->error("Failed to open file {} with result {} '{}'",
                        full_path, result.value(), result.message());
        return request::handle_error(404, acceptor);
    }

    std::string response{"HTTP/1.1 200 OK\r\n"
                         "Content-Type: text/html\r\n"
                         "Content-Length: " + std::to_string(file.size()) + "\r\n"
                         "\r\n"};

    if (auto result = acceptor.write(response); !result) {
        logger->error("Failed to send response header with result {} '{}'",
                        result.error().value(), result.error().message());
        return request::handle_error(500, acceptor);
    }

    return acceptor.splice(file, file.size());
}
} // namespace fd::example::http::handler

auto main(int argc, char** argv)->int {

    using namespace fd::example;

    std::signal(SIGTERM, signal_handler);
    std::signal(SIGINT, signal_handler);

    const utility::application_option port_option{"-p,--port", false};

    auto [logger, app] = fd::example::utility::init_app(
        argc, argv, {port_option}, "Test HTTP server");

    constexpr uint16_t default_http_port{8443};
    uint16_t port{default_http_port};

    try {
        if (app->count("--port") > 0) {
            port = app->get_option("--port")->as<uint16_t>();
        }
        else {
            logger->warn("Port option not found, using default port {}", default_http_port);
        }
    }
    catch (const CLI::OptionNotFound &onf) {
        logger->critical(onf.what());
        fmt::print(fg(fmt::color::crimson), "{}\n", onf.what());
        return EXIT_FAILURE;
    }

    fd::tcp_socket server{};
    g_server = &server;

    logger->info("Binding to :{}", port);
    fmt::println("Binding to :{}", port);

    auto result = server.bind(port);
    if (result) {
        logger->error("Bind failed; {} '{}'", result.value(), result.message());
    }
    else {
        fmt::println("Server bound to :{}", port);

        result = server.listen();
        if (result) {
            logger->error("Listen failed; {} '{}'", result.value(), result.message());
        }
        else {
            fmt::println("Server listening on :{}", port);

            auto accept_result = server.secure_accept();

            while (accept_result.has_value()) {
                std::thread([logger](fd::secure_acceptor&& acceptor) {
                    logger->info("New connection from {}:{}", acceptor.peer(), acceptor.peer_port());

                    if (auto const init_result = acceptor.init_context(default_certificate_path); !init_result) {
                        logger->error("Failed to initialize secure context, error is {}({})", 
                                        init_result.error().value(), init_result.error().message());

                        logger->info("Shutting down the acceptor!");
                    }
                    else {
                        std::array<char, 4096> receive_buffer{};

                        fd::acceptor &acceptor_ref{acceptor};

                        auto const read_result = acceptor_ref.read(receive_buffer);

                        if (!read_result) {
                            logger->error("Receive failed; {} '{}'",
                                read_result.error().value(), read_result.error().message());
                        }
                        else if (read_result.value() == 0) {
                            logger->info("Connection from {}:{} has been closed!",
                                acceptor.peer(), acceptor.peer_port());
                        }
                        else {
                            std::string_view received{receive_buffer.data(), static_cast<size_t>(read_result.value())};

                            logger->info("Received '{}'", received);

                            http::request request{};
                            request.set_get_handler(http::handler::get);

                            const fd::expected_result send_response = request.handle(received, std::move(acceptor), logger);

                            if (!send_response) {
                                logger->error("Response failed; {} '{}'",
                                    send_response.error().value(), send_response.error().message());
                            }
                            else {
                                logger->info("Sent '{}'", send_response.value());
                            }
                        }
                    }
                }, std::move(accept_result.value())).detach();

                accept_result = server.secure_accept();
            }

            logger->error("Accept failed; {} '{}'",
                accept_result.error().value(), accept_result.error().message());
        }
    }

    fmt::print(fg(fmt::color::green), "\nEXIT!\n");

    return EXIT_SUCCESS;
}
