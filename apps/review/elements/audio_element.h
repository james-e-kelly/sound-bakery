#pragma once

#include "elements/file_element.h"

class audio_element : public file_element
{
public:
    audio_element(const std::filesystem::path& filePath, const int64_t fileId);

	static auto can_handle_file(const std::filesystem::path& filePath) -> bool
    {
        const std::string extension = filePath.extension().string();

        const std::unordered_set<std::string> audioExtensions = {
            ".ac3",     ".a52", ".eac3",   ".mlp",     ".dts",  ".dts-hd", ".dtshd",
            ".true-hd", ".thd", ".truehd", ".thd+ac3", ".tta",  ".pcm",    ".wav",
            ".aiff",    ".aif", ".aifc",   ".amr",     ".awb",  ".au",     ".snd",
            ".lpcm",    ".ape", ".wv",     ".shn",     ".adts", ".adt",    ".mpa",
            ".m1a",     ".m2a", ".mp1",    ".mp2",     ".mp3",  ".m4a",    ".aac",
            ".flac",    ".oga", ".ogg",    ".opus",    ".spx",  ".mka",    ".weba",
            ".wma",     ".f4a", ".ra",     ".ram",     ".3ga",  ".3ga2",   ".ay",
            ".gbs",     ".gym", ".hes",    ".kss",     ".nsf",  ".nsfe",   ".sap",
            ".spc",     ".vgm", ".vgz",    ".m3u",     ".m3u8", ".pls",    ".cue"};

        return audioExtensions.contains(extension);
    }

    auto seek_to_position(double position) -> void override;

protected:
    auto render_element(const ImRect& elementRect) -> bool override;
    auto render_waveform() -> void;
    auto render_comments() -> bool;
    auto get_element_content_size(const ImVec2& parentSize) -> ImVec2 const override;

    auto handle_mouse_control() -> void;

    auto get_file_play_position() const -> double override;
    auto get_file_duration() const -> double override;

    auto play_file() -> void override;
    auto pause_file() -> void override;
    auto prev_frame() -> void override;
    auto next_frame() -> void override;
    auto get_is_playing() -> bool override;

private:
    auto get_audio_height(float width) -> float;

    gluten::background m_audioBackground;
    gluten::background m_loudnessBackground;
    gluten::background m_controlsBackground;

    gluten::layout m_layout = gluten::layout(gluten::layout_type::top_to_bottom, gluten::anchor_preset::stretch_full);
    gluten::layout m_waveformAndLoudnessLayout = gluten::layout(gluten::layout_type::left_to_right, gluten::anchor_preset::stretch_full);
};