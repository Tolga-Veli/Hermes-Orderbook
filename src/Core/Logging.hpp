#pragma once

#include <cstdio>
#include <format>
#include <print>
#include <source_location>
#include <string_view>
#include <utility>

namespace Hermes::core {
enum class LogLevel : std::uint8_t {
  Trace = 0,
  Debug,
  Info,
  Warn,
  Error,
  Fatal
};

namespace detail {

[[nodiscard]] constexpr std::string_view to_string(LogLevel level) noexcept {
  using enum LogLevel;

  switch (level) {
  case Trace:
    return "TRACE";
  case Debug:
    return "DEBUG";
  case Info:
    return "INFO";
  case Warn:
    return "WARN";
  case Error:
    return "ERROR";
  case Fatal:
    return "FATAL";
  default:
    return "Unknown";
  }
}

// ANSI codes for terminal colour
[[nodiscard]] constexpr std::string_view
to_ansi_color(LogLevel level) noexcept {
  using enum LogLevel;

  switch (level) {
  case Trace:
    return "\x1b[90m"; // Gray
  case Debug:
    return "\x1b[36m"; // Cyan
  case Info:
    return "\x1b[32m"; // Green
  case Warn:
    return "\x1b[33m"; // Yellow
  case Error:
    return "\x1b[31m"; // Red
  case Fatal:
    return "\x1b[41;97m"; // White text on red background
  default:
    return "";
  }
}

inline constexpr std::string_view k_ColorReset = "\x1b[0m";

class Logger {
public:
  static Logger &GetInstance() noexcept {
    static Logger instance;
    return instance;
  }

  template <typename... Args>
  void Log(LogLevel level, std::source_location loc,
           std::format_string<Args...> fmt, Args &&...args) {

    const std::string msg = std::format(fmt, std::forward<Args>(args)...);

    std::println("{}[{}]{} {} ({}:{})", to_ansi_color(level), to_string(level),
                 k_ColorReset, msg, loc.file_name(), loc.line());
  }

private:
  Logger() = default;
};
} // namespace detail
} // namespace Hermes::core

#define LOG(level, ...)                                                        \
  ::Hermes::core::detail::Logger::GetInstance().Log(                           \
      level, std::source_location::current(), __VA_ARGS__)

#define LOG_TRACE(...) LOG(::Hermes::core::LogLevel::Trace, __VA_ARGS__)
#define LOG_DEBUG(...) LOG(::Hermes::core::LogLevel::Debug, __VA_ARGS__)
#define LOG_INFO(...) LOG(::Hermes::core::LogLevel::Info, __VA_ARGS__)
#define LOG_WARN(...) LOG(::Hermes::core::LogLevel::Warn, __VA_ARGS__)
#define LOG_ERROR(...) LOG(::Hermes::core::LogLevel::Error, __VA_ARGS__)
#define LOG_FATAL(...) LOG(::Hermes::core::LogLevel::Fatal, __VA_ARGS__)
