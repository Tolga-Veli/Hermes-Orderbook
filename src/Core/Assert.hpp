#pragma once

#include "Logging.hpp"

#include <cstdlib>
#include <source_location>
#include <string_view>

#if defined(_MSC_VER)
#define HERMES_DEBUG_BREAK() __debugbreak()
#elif defined(__GNUC__) || defined(__clang__)
#define HERMES_DEBUG_BREAK() __builtin_trap()
#else
#include <csignal>
#define HERMES_DEBUG_BREAK() raise(SIGTRAP)
#endif

namespace Hermes::core::detail {

[[noreturn]] inline void AssertFail(std::string_view expr, std::string_view msg,
                                    std::source_location loc) {
  Logger::GetInstance().Log(LogLevel::Fatal, loc, "Assertion failed: ({}) {}",
                            expr, msg);
  HERMES_DEBUG_BREAK();
  std::abort();
}

[[noreturn]] inline void UnreachableFail(std::string_view msg,
                                         std::source_location loc) {
  Logger::GetInstance().Log(LogLevel::Fatal, loc,
                            "Unreachable code reached: {}", msg);
  HERMES_DEBUG_BREAK();
  std::abort();
}

} // namespace Hermes::core::detail

// Always active, in debug AND release. Use for conditions that must never
// be violated without immediately corrupting program state. If you're ever
// tempted to disable this in release "for performance", that's a sign the
// check belongs in HERMES_ASSERT instead.
#define HERMES_VERIFY(cond, msg)                                               \
  do {                                                                         \
    if (!(cond)) {                                                             \
      ::Hermes::core::detail::AssertFail(#cond, msg,                           \
                                         std::source_location::current());     \
    }                                                                          \
  } while (0)

// Compiled out entirely in release - the condition itself is never
// evaluated, so this costs nothing and won't trigger unused-variable
// warnings on release builds. Use for expensive or redundant sanity checks
// you only want while developing (bounds checks, invariant re-verification,
// etc).
//
// Define HERMES_FORCE_ASSERTS to keep these live in a release build
#if !defined(NDEBUG) || defined(HERMES_FORCE_ASSERTS)
#define HERMES_ASSERT(cond, msg) HERMES_VERIFY(cond, msg)
#define HERMES_ENABLE_ASSERTS 1
#else
#define HERMES_ASSERT(cond, msg) ((void)0)
#define HERMES_ENABLE_ASSERTS 0
#endif

#define UNREACHABLE(msg)                                                       \
  do {                                                                         \
    ::Hermes::core::detail::UnreachableFail(msg,                               \
                                            std::source_location::current());  \
  } while (false)
