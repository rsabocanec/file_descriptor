#ifndef FD_EXAMPLE_UTILITY_HPP
#define FD_EXAMPLE_UTILITY_HPP

#pragma once

#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <fmt/color.h>

#include <tuple>
#include <string_view>

namespace fd::example::utility {

    class application_option {
        std::string name_;
        bool required_{true};

    public:
        application_option(const std::string &name)
        : name_(name) {}

        application_option(const std::string &name, bool required)
        : name_(name)
        , required_(required) {}

        const std::string &name() const noexcept {
            return name_;
        }

        bool required() const noexcept {
            return required_;
        }
    };

    inline std::tuple<std::shared_ptr<spdlog::logger>, std::unique_ptr<CLI::App>> init_app(
        int argc, char **argv,
        const std::vector<application_option> &app_options,
        std::string_view app_description,
        std::string_view logger_name = "logger") {

        std::shared_ptr<spdlog::logger> logger{};

        try {
            logger = spdlog::basic_logger_mt(std::string(logger_name), std::string(argv[0]) + ".log");
        }
        catch (const spdlog::spdlog_ex &ex) {
            fmt::print(fg(fmt::color::red) | fmt::emphasis::bold, "Log initialization failed: {}\n", ex.what());
            logger = spdlog::default_logger();
        }

        auto app = std::make_unique<CLI::App>(std::string(app_description));

        for (const auto &option : app_options) {
            app->add_option(option.name())
            ->required(option.required());
        }

        app->add_flag_function("-v,--verbose", [logger](long int) {
            logger->set_level(spdlog::level::debug);
        }, "Set verbosity");

        try {
            app->parse(argc, argv);
        }
        catch(const CLI::ParseError &e) {
            logger->error("Error parsing command line arguments: {}", e.what());
            std::exit(app->exit(e));
        }

        return {logger, std::move(app)};
    }
} // namespace fd::example::utility

#endif //FD_EXAMPLE_UTILITY_HPP