#include "sound_bakery/core/error/error.h"

#include <spdlog/fmt/fmt.h>

namespace sbk
{
    auto to_string(sbk_status code) -> std::string_view
    {
        switch (code)
        {
            case SBK_SUCCESS:
                return "Success";
            case SBK_ERR_USER:
                return "User error";
            case SBK_ERR_INVALID_PARAMETER:
                return "Invalid parameter passed to the function";
            case SBK_ERR_ALREADY_INITIALIZED:
                return "The resource was already initialized and should not be initialized again";
            case SBK_ERR_CHEF:
                return "Error from Sound Chef";
            case SBK_ERR_CHEF_UNINITIALIZED:
                return "Sound Chef is not initialized";
            case SBK_ERR_BAKERY:
                return "Generic Sound Bakery error";
            case SBK_ERR_BAKERY_UNINITIALIZED:
                return "Sound Bakery is not initialized";
            case SBK_ERR_BAKERY_SERIALIZATION:
                return "Error occurred while serializing";
            case SBK_ERR_BAKERY_OBJECT_NOT_FOUND:
                return "Could not find an object with the ID or name";
            case SBK_ERR_BAKERY_OBJECT_EXISTS:
                return "An object with the ID or name already exists";
            case SBK_ERR_SYSTEM:
                return "The running system / computer encountered an error";
            case SBK_ERR_OUT_OF_MEMORY:
                return "Could not allocate memory or general memory error";
            case SBK_ERR_INVALID_FILE:
                return "File was invalid";
            case SBK_ERR_NULL:
                return "Tried to access a null variable";
            case SBK_ERR_FULL:
                return "The buffer or container was full and cannot accept more data";
            case SBK_ERR_EMPTY:
                return "The buffer or container was empty and nothing could be read";
            case SBK_ERR_TOO_LARGE:
                return "The request was too large and nothing could be written or read";
            case SBK_ERR_AT_END:
                return "At the end of the buffer and cannot go further";
            case SBK_ERR_UNINITIALIZED:
                return "The resource was not initialized";
            case SBK_ERR_INVALID_OPERATION:
                return "The operation is unsupported in this state";
            case SBK_ERR_NOT_FOUND:
                return "The resource was not found";
            case SBK_ERROR_MAX:
                break;
        }

        if (code < 0)
        {
            return ma_result_description(static_cast<ma_result>(code));
        }

        return "SBK_ERR_UNKNOWN";
    }

    namespace
    {
        // Formats and emits a single log line for a failure. Shared by log_error and make_error.
        auto emit(sbk_status code, std::string_view message, const std::source_location& location) -> void
        {
            if (code == SBK_SUCCESS)
            {
                return;
            }

            // Keep just the file name, not the full path, to keep log lines readable.
            std::string_view file = location.file_name();
            if (const std::size_t slash = file.find_last_of("/\\"); slash != std::string_view::npos)
            {
                file.remove_prefix(slash + 1);
            }

            const std::string formatted =
                message.empty()
                    ? fmt::format("{} ({}) [{}:{} {}]", to_string(code), static_cast<int>(code), file, location.line(),
                                  location.function_name())
                    : fmt::format("{} ({}): {} [{}:{} {}]", to_string(code), static_cast<int>(code), message, file,
                                  location.line(), location.function_name());

            sbk_log(MA_LOG_LEVEL_ERROR, formatted.c_str());
        }
    }  // namespace

    auto log_error(sbk_status code, std::string_view message, const std::source_location& location) -> void
    {
        emit(code, message, location);
    }

    auto make_error(sbk_status code, std::string_view message, const std::source_location& location) -> tl::unexpected<error>
    {
        // Log once, here, at the point the failure is first produced; the message is not carried in the error.
        emit(code, message, location);
        return tl::unexpected<error>(error(code, location));
    }
}  // namespace sbk
