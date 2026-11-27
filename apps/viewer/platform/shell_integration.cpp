#include "shell_integration.h"

#include "platform/audio_file_types.h"
#include "platform/directory_listing.h"

#include "fmt/format.h"

#if defined(_WIN32)
    #include <windows.h>
    #include <shellapi.h>
    #include <shlobj.h>
#endif

#if defined(_WIN32)

namespace
{
    constexpr const wchar_t* s_instanceMutexName   = L"Local\\JamesKellyAudio.SoundViewer.Instance";
    constexpr const wchar_t* s_listenerWindowClass = L"JamesKellyAudio.SoundViewer.Listener";
    constexpr ULONG_PTR s_forwardedFilesMessageId  = 0x53564657;  // "SVFW"

    constexpr const wchar_t* s_progId              = L"SoundViewer.AudioFile";
    constexpr const wchar_t* s_applicationName     = L"Sound Viewer";
    constexpr const wchar_t* s_capabilitiesKeyPath = L"Software\\SoundViewer\\Capabilities";

    std::function<void(std::vector<std::filesystem::path>)> s_onFilesReceived;
    HWND s_listenerWindow  = nullptr;
    HANDLE s_instanceMutex = nullptr;  //< Never closed: it marks this process as the running instance for its whole lifetime

    auto get_executable_path() -> std::filesystem::path
    {
        std::wstring buffer(MAX_PATH, L'\0');
        while (true)
        {
            const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
            if (length < buffer.size())
            {
                buffer.resize(length);
                return buffer;
            }
            buffer.resize(buffer.size() * 2);
        }
    }

    auto get_open_command() -> std::wstring
    {
        return L"\"" + get_executable_path().native() + L"\" \"%1\"";
    }

    auto get_applications_key_path() -> std::wstring
    {
        return L"Software\\Classes\\Applications\\" + get_executable_path().filename().native();
    }

    /**
     * @brief Files are sent as UTF-8 paths separated by newlines, which can't appear in a Windows path.
     */
    auto encode_files(const std::vector<std::filesystem::path>& files) -> std::string
    {
        std::string encoded;
        for (const std::filesystem::path& file : files)
        {
            encoded += path_to_utf8(file);
            encoded += '\n';
        }
        return encoded;
    }

    auto decode_files(std::string_view encoded) -> std::vector<std::filesystem::path>
    {
        std::vector<std::filesystem::path> files;
        while (!encoded.empty())
        {
            const std::size_t lineEnd = encoded.find('\n');
            const std::string_view line = encoded.substr(0, lineEnd);
            if (!line.empty())
            {
                files.push_back(utf8_to_path(line));
            }
            if (lineEnd == std::string_view::npos)
            {
                break;
            }
            encoded.remove_prefix(lineEnd + 1);
        }
        return files;
    }

    LRESULT CALLBACK listener_window_proc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == WM_COPYDATA)
        {
            const COPYDATASTRUCT* const copyData = reinterpret_cast<const COPYDATASTRUCT*>(lParam);
            if (copyData && copyData->dwData == s_forwardedFilesMessageId)
            {
                if (s_onFilesReceived)
                {
                    s_onFilesReceived(decode_files(std::string_view(static_cast<const char*>(copyData->lpData), copyData->cbData)));
                }
                return TRUE;
            }
        }

        return DefWindowProcW(window, message, wParam, lParam);
    }

    auto set_registry_string(HKEY root, const std::wstring& keyPath, const wchar_t* valueName, const std::wstring& value) -> LSTATUS
    {
        HKEY key = nullptr;
        LSTATUS status = RegCreateKeyExW(root, keyPath.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, nullptr, &key, nullptr);
        if (status == ERROR_SUCCESS)
        {
            status = RegSetValueExW(key, valueName, 0, REG_SZ, reinterpret_cast<const BYTE*>(value.c_str()), static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t)));
            RegCloseKey(key);
        }
        return status;
    }

    auto set_registry_empty_value(HKEY root, const std::wstring& keyPath, const wchar_t* valueName) -> LSTATUS
    {
        HKEY key = nullptr;
        LSTATUS status = RegCreateKeyExW(root, keyPath.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, nullptr, &key, nullptr);
        if (status == ERROR_SUCCESS)
        {
            status = RegSetValueExW(key, valueName, 0, REG_NONE, nullptr, 0);
            RegCloseKey(key);
        }
        return status;
    }

    auto get_registry_string(HKEY root, const std::wstring& keyPath, const wchar_t* valueName) -> std::optional<std::wstring>
    {
        DWORD size = 0;
        if (RegGetValueW(root, keyPath.c_str(), valueName, RRF_RT_REG_SZ, nullptr, nullptr, &size) != ERROR_SUCCESS)
        {
            return std::nullopt;
        }

        std::wstring value(size / sizeof(wchar_t), L'\0');
        if (RegGetValueW(root, keyPath.c_str(), valueName, RRF_RT_REG_SZ, nullptr, value.data(), &size) != ERROR_SUCCESS)
        {
            return std::nullopt;
        }

        value.resize(std::wcslen(value.c_str()));
        return value;
    }

    auto registry_error(std::string_view action, LSTATUS status) -> std::string
    {
        return fmt::format("{}: {}", action, std::system_category().message(static_cast<int>(status)));
    }
}  // namespace

auto shell_integration::forward_to_running_instance(const std::vector<std::filesystem::path>& files) -> bool
{
    s_instanceMutex          = CreateMutexW(nullptr, FALSE, s_instanceMutexName);
    const DWORD createResult = GetLastError();

    if (!s_instanceMutex || createResult != ERROR_ALREADY_EXISTS)
    {
        return false;
    }

    constexpr int s_attempts         = 50;
    constexpr DWORD s_attemptDelayMs = 100;

    for (int attempt = 0; attempt < s_attempts; ++attempt)
    {
        if (const HWND listener = FindWindowExW(HWND_MESSAGE, nullptr, s_listenerWindowClass, nullptr))
        {
            DWORD runningProcessId = 0;
            GetWindowThreadProcessId(listener, &runningProcessId);
            AllowSetForegroundWindow(runningProcessId);

            const std::string encoded = encode_files(files);

            COPYDATASTRUCT copyData{};
            copyData.dwData = s_forwardedFilesMessageId;
            copyData.cbData = static_cast<DWORD>(encoded.size());
            copyData.lpData = const_cast<char*>(encoded.data());

            DWORD_PTR result = 0;
            if (SendMessageTimeoutW(listener, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&copyData), SMTO_ABORTIFHUNG, 5000, &result) && result == TRUE)
            {
                return true;
            }
        }

        Sleep(s_attemptDelayMs);
    }

    return false;
}

auto shell_integration::start_accepting_forwarded_launches(std::function<void(std::vector<std::filesystem::path>)> onFilesReceived) -> void
{
    s_onFilesReceived = std::move(onFilesReceived);

    if (s_listenerWindow)
    {
        return;
    }

    WNDCLASSEXW windowClass{};
    windowClass.cbSize        = sizeof(windowClass);
    windowClass.lpfnWndProc   = listener_window_proc;
    windowClass.hInstance     = GetModuleHandleW(nullptr);
    windowClass.lpszClassName = s_listenerWindowClass;
    RegisterClassExW(&windowClass);

    s_listenerWindow = CreateWindowExW(0, s_listenerWindowClass, s_applicationName, 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, windowClass.hInstance, nullptr);
}

auto shell_integration::stop_accepting_forwarded_launches() -> void
{
    if (s_listenerWindow)
    {
        DestroyWindow(s_listenerWindow);
        s_listenerWindow = nullptr;
    }
    s_onFilesReceived = nullptr;
}

auto shell_integration::get_registration_state() -> registration_state
{
    const std::wstring commandKeyPath = std::wstring(L"Software\\Classes\\") + s_progId + L"\\shell\\open\\command";

    const std::optional<std::wstring> command = get_registry_string(HKEY_CURRENT_USER, commandKeyPath, nullptr);
    if (!command.has_value())
    {
        return registration_state::not_registered;
    }

    return _wcsicmp(command->c_str(), get_open_command().c_str()) == 0 ? registration_state::registered : registration_state::registered_to_other_copy;
}

auto shell_integration::register_file_types() -> tl::expected<void, std::string>
{
    const std::filesystem::path executable = get_executable_path();
    const std::wstring openCommand         = get_open_command();
    const std::wstring progIdKeyPath       = std::wstring(L"Software\\Classes\\") + s_progId;
    const std::wstring applicationsKeyPath = get_applications_key_path();

    // Without MultiSelectModel=Player, opening several files launches one process per file.
    const std::pair<std::wstring, std::pair<const wchar_t*, std::wstring>> stringValues[] = {
        {progIdKeyPath, {nullptr, L"Audio File"}},
        {progIdKeyPath + L"\\DefaultIcon", {nullptr, L"\"" + executable.native() + L"\",0"}},
        {progIdKeyPath + L"\\shell\\open", {L"MultiSelectModel", L"Player"}},
        {progIdKeyPath + L"\\shell\\open\\command", {nullptr, openCommand}},
        {applicationsKeyPath, {L"FriendlyAppName", s_applicationName}},
        {applicationsKeyPath + L"\\shell\\open", {L"MultiSelectModel", L"Player"}},
        {applicationsKeyPath + L"\\shell\\open\\command", {nullptr, openCommand}},
        {s_capabilitiesKeyPath, {L"ApplicationName", s_applicationName}},
        {s_capabilitiesKeyPath, {L"ApplicationDescription", L"Browse and audition audio files"}},
        {L"Software\\RegisteredApplications", {s_applicationName, s_capabilitiesKeyPath}},
    };

    for (const auto& [keyPath, value] : stringValues)
    {
        if (const LSTATUS status = set_registry_string(HKEY_CURRENT_USER, keyPath, value.first, value.second); status != ERROR_SUCCESS)
        {
            return tl::unexpected(registry_error("Couldn't register Sound Viewer", status));
        }
    }

    for (const std::wstring_view extension : s_supportedAudioExtensions)
    {
        const std::wstring extensionString(extension);

        LSTATUS status = set_registry_empty_value(HKEY_CURRENT_USER, L"Software\\Classes\\" + extensionString + L"\\OpenWithProgids", s_progId);
        if (status == ERROR_SUCCESS)
        {
            status = set_registry_string(HKEY_CURRENT_USER, applicationsKeyPath + L"\\SupportedTypes", extensionString.c_str(), L"");
        }
        if (status == ERROR_SUCCESS)
        {
            status = set_registry_string(HKEY_CURRENT_USER, std::wstring(s_capabilitiesKeyPath) + L"\\FileAssociations", extensionString.c_str(), s_progId);
        }
        if (status != ERROR_SUCCESS)
        {
            return tl::unexpected(registry_error(fmt::format("Couldn't register {}", path_to_utf8(extensionString)), status));
        }
    }

    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
    return {};
}

auto shell_integration::unregister_file_types() -> tl::expected<void, std::string>
{
    const auto delete_tree = [](const std::wstring& keyPath) -> LSTATUS
    {
        const LSTATUS status = RegDeleteTreeW(HKEY_CURRENT_USER, keyPath.c_str());
        return status == ERROR_FILE_NOT_FOUND ? ERROR_SUCCESS : status;
    };

    for (const std::wstring& keyPath : {std::wstring(L"Software\\Classes\\") + s_progId, get_applications_key_path(), std::wstring(L"Software\\SoundViewer")})
    {
        if (const LSTATUS status = delete_tree(keyPath); status != ERROR_SUCCESS)
        {
            return tl::unexpected(registry_error("Couldn't unregister Sound Viewer", status));
        }
    }

    (void)RegDeleteKeyValueW(HKEY_CURRENT_USER, L"Software\\RegisteredApplications", s_applicationName);

    for (const std::wstring_view extension : s_supportedAudioExtensions)
    {
        (void)RegDeleteKeyValueW(HKEY_CURRENT_USER, (L"Software\\Classes\\" + std::wstring(extension) + L"\\OpenWithProgids").c_str(), s_progId);
    }

    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
    return {};
}

auto shell_integration::open_default_apps_settings() -> void
{
    ShellExecuteW(nullptr, L"open", L"ms-settings:defaultapps?registeredAppUser=Sound%20Viewer", nullptr, nullptr, SW_SHOWNORMAL);
}

#else

auto shell_integration::forward_to_running_instance(const std::vector<std::filesystem::path>& files) -> bool
{
    return false;
}

auto shell_integration::start_accepting_forwarded_launches(std::function<void(std::vector<std::filesystem::path>)> onFilesReceived) -> void
{
}

auto shell_integration::stop_accepting_forwarded_launches() -> void
{
}

auto shell_integration::get_registration_state() -> registration_state
{
    return registration_state::not_registered;
}

auto shell_integration::register_file_types() -> tl::expected<void, std::string>
{
    return tl::unexpected(std::string("Only supported on Windows"));
}

auto shell_integration::unregister_file_types() -> tl::expected<void, std::string>
{
    return tl::unexpected(std::string("Only supported on Windows"));
}

auto shell_integration::open_default_apps_settings() -> void
{
}

#endif
