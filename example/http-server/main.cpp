#include <tcp_socket.h>
#include <file.h>

#include "http/request.h"
#include "../utility.hpp"

#include <array>
#include <span>
#include <atomic>
#include <csignal>

namespace {
    volatile std::sig_atomic_t signal_status;

    fd::tcp_socket *g_server{nullptr};
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


auto main(int argc, char** argv)->int {

    using namespace fd::example;

    std::signal(SIGTERM, signal_handler);
    std::signal(SIGINT, signal_handler);

    const utility::application_option port_option{"-p,--port", false};

    auto [logger, app] = fd::example::utility::init_app(
        argc, argv, {port_option}, "Test HTTP server");

    constexpr uint16_t default_http_port{8080};
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

            auto accept_result = server.accept();

            while (accept_result.has_value()) {
                std::thread([logger](fd::acceptor&& acceptor) {
                    logger->info("New connection from {}:{}", acceptor.peer(), acceptor.peer_port());

                    std::array<char, 4096> receive_buffer{};

                    auto const read_result = acceptor.read(receive_buffer);

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

                        http::request request{logger};

                        const fd::expected_result send_response = request.handle(received, acceptor);

                        if (!send_response) {
                            logger->error("Response failed; {} '{}'",
                                send_response.error().value(), send_response.error().message());
                        }
                        else {
                            logger->info("Sent '{}'", send_response.value());
                        }
                    }

                    logger->info("Shutting down the acceptor!");

                    [[maybe_unused]] auto shutdown_result = acceptor.shutdown();

                    logger->info("Client {}:{} disconnected!", acceptor.peer(), acceptor.peer_port());

                    logger->flush();

                }, std::move(accept_result.value())).detach();

                accept_result = server.accept();
            }

            logger->error("Accept failed; {} '{}'",
                accept_result.error().value(), accept_result.error().message());
        }
    }

    fmt::print(fg(fmt::color::green), "\nEXIT!\n");

    return EXIT_SUCCESS;
}
