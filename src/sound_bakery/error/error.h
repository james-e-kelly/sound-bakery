#pragma once

#include "sound_chef/sound_chef_common.h"  // sbk_status and its codes
#include "tl/expected.hpp"

#include <source_location>
#include <string>
#include <string_view>
#include <utility>

/**
 * @file error.h
 * @brief Small, self-describing errors for the C++ engine layer.
 *
 * The C ABI (all @c sc_* and @c sbk_* functions) continues to speak @c sbk_status.
 * Internally, C++ code carries a @ref sbk::error, which pairs that code with the
 * @c std::source_location where the failure originated.
 *
 * @ref sbk::error deliberately holds no owned string: it is trivially copyable and
 * allocation-free, so @ref sbk::result is cheap to return everywhere (important for a
 * games/console runtime, and required for a possible @c -fno-exceptions build). The
 * human-readable message is *logged* at the origin (see @ref sbk::make_error /
 * @ref sbk::log_error) rather than carried up the stack.
 *
 * Errors are logged exactly once, at the point they are first produced. Propagating an
 * existing error up the call stack does not log again, so a single failure yields a
 * single log line.
 */

namespace sbk
{
    /**
     * @brief Returns a human-readable name for an @c sbk_status code, e.g. "SBK_ERR_INVALID_FILE".
     */
    [[nodiscard]] auto to_string(sbk_status code) -> std::string_view;

    /**
     * @brief A failure value: the @c sbk_status code plus where it came from.
     *
     * Used as the error type of @ref sbk::result. Trivially copyable and allocation-free.
     * The descriptive message is logged at origin, not stored here.
     */
    class error
    {
    public:
        error() = default;
        error(sbk_status code, const std::source_location& location) : m_code(code), m_location(location) {}

        [[nodiscard]] auto code() const -> sbk_status { return m_code; }
        [[nodiscard]] auto location() const -> const std::source_location& { return m_location; }

    private:
        sbk_status m_code = SBK_ERR_SYSTEM;
        std::source_location m_location{};
    };

    /**
     * @brief Logs a failure once, at its origin, through the Sound Bakery logger.
     *
     * A no-op when @p code is @c SBK_SUCCESS. The message is only used for the log line.
     * Prefer the @c SBK_* macros in result.h over calling this directly.
     */
    auto log_error(sbk_status code, std::string_view message = {}, const std::source_location& location = std::source_location::current()) -> void;

    /**
     * @brief Logs a failure once and wraps it for returning from a @ref result.
     *
     * The message is logged, not carried in the returned @ref error.
     *
     * @code
     *   if (!file_exists) return sbk::make_error(SBK_ERR_INVALID_FILE, path.string());
     * @endcode
     */
    [[nodiscard]] auto make_error(sbk_status code, std::string_view message = {}, const std::source_location& location = std::source_location::current()) -> tl::unexpected<error>;

    /**
     * @brief The result of a fallible operation: a value of type @p T, or an @ref sbk::error.
     *
     * @c result<void> represents an operation that either succeeds or fails with no value.
     * @c tl::expected is already marked nodiscard, so ignoring a returned result warns.
     *
     * Prefer this over the raw @c sbk_status code for internal C++ code; see error/README.md
     * and the control-flow macros in result.h.
     */
    template <class T = void>
    using result = tl::expected<T, error>;

    /**
     * @brief Success value for a @c result<void> function.
     */
    [[nodiscard]] inline auto ok() -> result<void> { return {}; }

    /**
     * @brief Collapses a rich @ref result back into a C-ABI @c sbk_status code.
     *
     * Use this to bridge a @c sbk::result<T> function to a caller that returns @c sbk_status,
     * e.g. at the C API boundary: `return sbk::to_status(do_the_work());`.
     */
    template <class T>
    [[nodiscard]] auto to_status(const result<T>& r) -> sbk_status
    {
        return r.has_value() ? SBK_SUCCESS : r.error().code();
    }
}  // namespace sbk
