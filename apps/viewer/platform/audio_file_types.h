#pragma once

#include <algorithm>
#include <array>
#include <cwctype>
#include <filesystem>
#include <string>
#include <string_view>

/**
 * @brief Extensions both playback and the waveform can decode.
 */
inline constexpr std::array<std::wstring_view, 11> s_supportedAudioExtensions = {L".wav", L".wave", L".bwf", L".amb", L".w64", L".rf64", L".flac", L".mp3", L".ogg", L".oga", L".opus"};

inline auto is_supported_audio_file(const std::filesystem::path& path) -> bool
{
    std::wstring extension = path.extension().wstring();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](wchar_t character) { return static_cast<wchar_t>(std::towlower(character)); });

    return std::find(s_supportedAudioExtensions.begin(), s_supportedAudioExtensions.end(), extension) != s_supportedAudioExtensions.end();
}
