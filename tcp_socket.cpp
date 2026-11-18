#include "tcp_socket.h"

#include <cassert>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


namespace fd {

std::error_code tcp_socket::create() noexcept {
    auto result = close();
    if (!result) {
        descriptor_ = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (descriptor_.load() == -1) {
            result = std::make_error_code(static_cast<std::errc>(errno));
        }
    }

    return result;
}

std::error_code tcp_socket::bind(std::string_view address, uint16_t port) noexcept {
    if (descriptor_.load() == -1) {
        if (auto const result = create()) {
            return result;
        }
    }

    if (port == 0) {
        return std::make_error_code(std::errc::invalid_argument);
    }

    struct sockaddr_in server_address{
        .sin_family = AF_INET,
        .sin_port = ::htons(port),
        .sin_addr = {.s_addr = ::htonl(INADDR_ANY)}};

    if (!address.empty()) {
        if (::inet_pton(AF_INET, address.data(), &server_address.sin_addr) == -1) {
            return std::make_error_code(static_cast<std::errc>(errno));
        }
    }

    if (::bind(descriptor_.load(),
                    static_cast<const struct sockaddr *>(static_cast<const void*>(&server_address)),
                    sizeof(server_address)) == -1) {
        return std::make_error_code(static_cast<std::errc>(errno));
    }

    return std::error_code{};
}

std::expected<acceptor, std::error_code> tcp_socket::accept() const noexcept {
    if (descriptor_.load() == -1) {
        return std::unexpected(std::make_error_code(std::errc::bad_file_descriptor));
    }

    struct sockaddr_in client_address{};
    socklen_t client_address_length{sizeof(client_address)};

    auto const result =
        ::accept(descriptor_.load(),
            static_cast<sockaddr *>(static_cast<void *>(&client_address)), &client_address_length);

    if (result == -1) {
        return std::unexpected(std::make_error_code(static_cast<std::errc>(errno)));
    }

    std::array<char, INET_ADDRSTRLEN> buffer{};
    ::inet_ntop(AF_INET, static_cast<const void *>(&client_address.sin_addr),
                buffer.data(), buffer.max_size());

    std::string peer_address{buffer.data()};

    return acceptor{result, std::move(peer_address), ::ntohs(client_address.sin_port)};
}

std::expected<secure_acceptor, std::error_code> tcp_socket::secure_accept() const noexcept {
    if (descriptor_.load() == -1) {
        return std::unexpected(std::make_error_code(std::errc::bad_file_descriptor));
    }

    struct sockaddr_in client_address{};
    socklen_t client_address_length{sizeof(client_address)};

    auto const result =
        ::accept(descriptor_.load(),
            static_cast<sockaddr *>(static_cast<void *>(&client_address)), &client_address_length);

    if (result == -1) {
        return std::unexpected(std::make_error_code(static_cast<std::errc>(errno)));
    }

    std::array<char, INET_ADDRSTRLEN> buffer{};
    ::inet_ntop(AF_INET, static_cast<const void *>(&client_address.sin_addr),
                buffer.data(), buffer.max_size());

    std::string peer_address{buffer.data()};

    return secure_acceptor{result, std::move(peer_address), ::ntohs(client_address.sin_port)};
}

std::error_code tcp_socket::connect(std::string_view address, uint16_t port) noexcept {
    if (descriptor_.load() == -1) {
        if (auto const result = create()) {
            return result;
        }
    }

    if (address.empty()) {
        return std::make_error_code(std::errc::invalid_argument);
    }

    if (port == 0) {
        return std::make_error_code(std::errc::invalid_argument);
    }

    struct sockaddr_in server_address{.sin_family = AF_INET, .sin_port = ::htons(port)};

    if (::inet_pton(AF_INET, address.data(), &server_address.sin_addr) == -1) {
        return std::make_error_code(static_cast<std::errc>(errno));
    }

    if (::connect(descriptor_.load(),
                    static_cast<const struct sockaddr *>(static_cast<const void*>(&server_address)),
                    sizeof(server_address)) == -1) {
        return std::make_error_code(static_cast<std::errc>(errno));
    }

    return {};
}

std::atomic<std::size_t> secure::context::instance_count_{};
std::once_flag secure::context::init_flag_;
std::once_flag secure::context::deinit_flag_;

struct secure::context::implementation {
    SSL_CTX *ssl_ctx_{nullptr};
    SSL *ssl_{nullptr};

    implementation() {
        ssl_ctx_ = ::SSL_CTX_new(TLS_method());
    }

    ~implementation() {
        [[maybe_unused]] auto const result = destroy_ssl();

        if (ssl_ctx_) {
            ::SSL_CTX_free(ssl_ctx_);
        }
    }

    [[nodiscard]] int32_t destroy_ssl() {
        int32_t result{};

        if (ssl_) {
            result = ::SSL_shutdown(ssl_);
            ::SSL_free(ssl_);
            ssl_ = nullptr;
        }

        return result;
    }

    [[nodiscard]] expected_result init(int32_t descriptor, std::string_view certificate_path) noexcept {
        if (ssl_ctx_ == nullptr) {
            return std::unexpected(std::make_error_code(std::errc::not_enough_memory));
        }

        auto const options = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1;
        auto const result = ::SSL_CTX_set_options(ssl_ctx_, options);
        assert(result & options == options);

        if (result & options != options) {
            return std::unexpected(std::make_error_code(std::errc::operation_not_permitted));
        }

        if (auto const result = ::SSL_CTX_use_certificate_file(ssl_ctx_, certificate_path.data(), SSL_FILETYPE_PEM); result != 1) {
            return std::unexpected(std::make_error_code(std::errc::no_such_file_or_directory));
        }

        if (auto const result = ::SSL_CTX_use_PrivateKey_file(ssl_ctx_, certificate_path.data(), SSL_FILETYPE_PEM); result != 1) {
            return std::unexpected(std::make_error_code(std::errc::no_such_file_or_directory));
        }

        if (ssl_ != nullptr) {
            ::SSL_free(ssl_);
        }

        ssl_ = ::SSL_new(ssl_ctx_);

        if (ssl_ == nullptr) {
            return std::unexpected(std::make_error_code(std::errc::not_enough_memory));
        }

        if (descriptor == -1) {
            return std::unexpected(std::make_error_code(std::errc::bad_file_descriptor));
        }

        if (auto result = ::SSL_set_fd(ssl_, descriptor); result != 1) {
            result = destroy_ssl();
            return std::unexpected(std::make_error_code(std::errc::bad_file_descriptor));
        }

        if (auto result = ::SSL_accept(ssl_); result != 1) {
            result = destroy_ssl();
            return std::unexpected(std::make_error_code(std::errc::operation_not_supported));
        }

        return {};
    }

    [[nodiscard]] expected_result read(std::span<std::byte> buffer) const noexcept {
        if (ssl_ != nullptr) {
            std::size_t bytes_read{};
            auto const result =
                ::SSL_read_ex(ssl_,
                              static_cast<void*>(buffer.data()),
                              static_cast<std::size_t>(buffer.size()),
                              &bytes_read);

            return result > 0 ? expected_result{static_cast<int32_t>(bytes_read)}
                              : std::unexpected(std::make_error_code(std::errc::invalid_argument));
        }

        return std::unexpected(std::make_error_code(std::errc::bad_file_descriptor));
    }

    [[nodiscard]] expected_result write(std::span<const std::byte> buffer) const noexcept {
        if (ssl_ != nullptr) {
            std::size_t bytes_written{};
            auto const result =
                ::SSL_write_ex(ssl_,
                               static_cast<const void*>(buffer.data()),
                               static_cast<std::size_t>(buffer.size()),
                               &bytes_written);

            return result > 0 ? expected_result{static_cast<int32_t>(bytes_written)}
                              : std::unexpected(std::make_error_code(std::errc::invalid_argument));
        }

        return std::unexpected(std::make_error_code(std::errc::bad_file_descriptor));
    }
};

secure::context::context() {
    if (instance_count_++ == 0) {
        std::call_once(init_flag_, []() {
            SSL_load_error_strings();
            SSL_library_init();
            OpenSSL_add_all_algorithms();
        });
    }

    impl_ = std::make_unique<implementation>();
}

secure::context::~context() {
    if (--instance_count_ == 0) {
        std::call_once(deinit_flag_, []() {
            EVP_cleanup();
            ERR_free_strings();
        });
    }
}

expected_result secure::context::init(int32_t descriptor, std::string_view certificate_path) noexcept {
    return impl_->init(descriptor, certificate_path);
}

expected_result secure::context::read(std::span<std::byte> buffer) const noexcept {
    return impl_->read(buffer);
}

expected_result secure::context::write(std::span<const std::byte> buffer) const noexcept {
    return impl_->write(buffer);
}

} // namespace fd