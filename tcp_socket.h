/*
Copyright (c) 2025 robert.sabocanec@gmail.com

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef FD_TCP_SOCKET_H
#define FD_TCP_SOCKET_H

#pragma once

#include "stream_socket.h"

#include <mutex>

namespace fd {

namespace secure {
class context {
    struct implementation;
    std::unique_ptr<implementation> impl_;

public:
    context();
    ~context();

    context(const context&) = delete;
    context(context&&) = default;

    context& operator=(const context&) = delete;
    context& operator=(context&&) = default;

    [[nodiscard]] expected_result init(int32_t descriptor, std::string_view certificate_path) noexcept;

    [[nodiscard]] expected_result read(std::span<std::byte> buffer) const noexcept;
    [[nodiscard]] expected_result write(std::span<const std::byte> buffer) const noexcept;

private:
    static std::once_flag init_flag_;
    static std::once_flag deinit_flag_;
    static std::atomic<std::size_t> instance_count_;
};
} // namespace secure

class secure_acceptor : public acceptor {
    secure::context context_{};
public:
    secure_acceptor() = delete;

    explicit secure_acceptor(int32_t descriptor) noexcept
    : acceptor(descriptor) {
    }

    explicit secure_acceptor(int32_t descriptor, std::string&& peer_address, uint16_t peer_port) noexcept
    : acceptor(descriptor, std::forward<std::string>(peer_address), peer_port) {
    }

    secure_acceptor(const secure_acceptor&) = delete;
    secure_acceptor(secure_acceptor&&) = default;

    secure_acceptor& operator=(const secure_acceptor&) = delete;
    secure_acceptor& operator=(secure_acceptor&&) = default;

    [[nodiscard]] expected_result init_context(int32_t descriptor, std::string_view certificate_path) noexcept {
        return context_.init(descriptor_.load(), certificate_path);
    }

    [[nodiscard]] expected_result read(std::span<std::byte> buffer) const noexcept final override {
        return context_.read(buffer);
    }

    [[nodiscard]] expected_result write(std::span<const std::byte> buffer) const noexcept final override {
        return context_.write(buffer);
    }
};


class tcp_socket : public stream_socket {
public:
    tcp_socket() = default;

    [[nodiscard]] std::error_code bind(std::string_view address) noexcept final {
        auto const result = parse_ip_address(address);
        if (!result) {
            return result.error();
        }

        return bind(std::get<0>(result.value()), std::get<1>(result.value()));
    }

    [[nodiscard]] std::error_code bind(uint16_t port) noexcept {
        return bind({}, port);
    }

    [[nodiscard]] std::error_code bind(std::string_view address, uint16_t port) noexcept;

    [[nodiscard]] std::expected<acceptor, std::error_code> accept() const noexcept final;

    [[nodiscard]] std::expected<secure_acceptor, std::error_code> secure_accept() const noexcept;

    [[nodiscard]] std::error_code connect(std::string_view address) noexcept final {
        auto const result = parse_ip_address(address);
        if (!result) {
            return result.error();
        }

        return connect(std::get<0>(result.value()), std::get<1>(result.value()));
    }

    [[nodiscard]] std::error_code connect(std::string_view address, uint16_t port) noexcept;

protected:
    [[nodiscard]] std::error_code create() noexcept final;
};
} // namespacefd

#endif //FD_TCP_SOCKET_H
