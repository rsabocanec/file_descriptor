#include <tcp_socket.h>
#include <file.h>

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

namespace {
    class http_request {
        std::string method_{};
        std::string path_{};
        std::string version_{};

        std::vector<std::pair<std::string, std::string>> headers_{};
        std::string body_{};

    public:
        http_request() = default;

        std::string handle(std::string_view request) noexcept {
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

    private:
        [[nodiscard]] bool parse(std::string_view request) noexcept {

            std::istringstream iss{std::string(request)};

            iss >> method_;
            iss >> path_;
            iss >> version_;

            return true;
        }
    };
}

auto main(int argc, char** argv)->int {

    std::signal(SIGTERM, signal_handler);
    std::signal(SIGINT, signal_handler);

    const fd::example::utility::application_option port_option{"-p,--port", false};

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

                    std::array<char, 4096> request{};

                    auto const read_result = acceptor.read(request);

                    if (!read_result) {
                        logger->error("Receive failed; {} '{}'",
                            read_result.error().value(), read_result.error().message());
                    }
                    else if (read_result.value() == 0) {
                        logger->info("Connection from {}:{} has been closed!",
                            acceptor.peer(), acceptor.peer_port());
                    }
                    else {
                        logger->info("Received '{}'", std::string_view(request.cbegin(), read_result.value()));

                        http_request http_request{};

                        const std::string response = http_request.handle(
                            std::string_view(request.cbegin(), read_result.value()));

                        auto const write_result = acceptor.write(response.cbegin(), response.cend());

                        if (!write_result) {
                            logger->error("Response failed; {} '{}'",
                                write_result.error().value(), write_result.error().message());
                        }
                        else {
                            logger->info("Sent '{}'", response);
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

#if 0
    fd::file file{};

    auto result = file.open(filename, fd::open_flags::read_only);
    if (result) {
        logger->error("Failed to open file {} with result {} '{}'",
            filename, result.value(), result.message());
        return result.value();
    }

    fd::tcp_socket client{};

    result = client.connect(address, port);
    if (result) {
        logger->error("Connect to {}:{} failed; {}", address, port, result.message());
    }
    else {
        auto const splice_result = client.splice(file, file.size());

        if (!splice_result) {
            logger->error("Splice failed; {}", splice_result.error().message());
        }
        else {
            logger->info("Spliced {} bytes from file {} to TCP socket {}:{}", splice_result.value(), filename, address, port);
        }

        fmt::print(fg(fmt::color::green), "\nDisconnecting!\n");

        result = client.disconnect();
        if (result) {
            logger->error("Disconnect failed; {}", result.message());
        }
    }

    result = file.close();
    if (result) {
        logger->error("Failed to close file {} with error {} '{}'",
            filename, result.value(), result.message());
    }
#endif

    fmt::print(fg(fmt::color::green), "\nEXIT!\n");

    return EXIT_SUCCESS;
}
