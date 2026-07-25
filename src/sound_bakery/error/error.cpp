#include "sound_bakery/error/error.h"

#include "sound_bakery/sound_bakery.h"  // sbk_log + ma_log_level

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
            case SBK_ERR_CHEF:
                return "Error from Sound Chef";
            case SBK_ERR_CHEF_UNITIALIZED:
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
            case SBK_ERROR_MAX:
                break;
        }

        // Negative codes are miniaudio (ma_result) errors surfaced through sbk_status.
        return code < 0 ? "SBK_ERR_MINIAUDIO" : "SBK_ERR_UNKNOWN";
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
