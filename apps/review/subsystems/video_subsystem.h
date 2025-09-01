#pragma once

#include "pch.h"

#include "mpv/client.h"
#include "mpv/render_gl.h"
#include "mpv/render.h"
#include "sound_chef/sound_chef.h"

class video_subsystem : public gluten::subsystem
{
public:
    video_subsystem(gluten::app* appOwner) : gluten::subsystem(appOwner) {}
    ~video_subsystem();

    auto load_video(const std::filesystem::path& absoluteFilePath) -> void;
    auto get_video_texture(const std::filesystem::path& file) const -> uint32_t;

    auto play_video(const std::filesystem::path& absoluteFilePath) -> void;
    auto pause_video(const std::filesystem::path& absoluteFilePath) -> void;

    auto get_video_play_position(const std::filesystem::path& absoluteFilePath) -> double;
    auto get_video_duration(const std::filesystem::path& absoluteFilePath) -> double;

    auto set_video_play_position(const std::filesystem::path& absoluteFilePath, double position) -> void;
    auto set_video_next_frame(const std::filesystem::path& absoluteFilePath) -> void;
    auto set_video_prev_frame(const std::filesystem::path& absoluteFilePath) -> void;

    auto pre_init(int ArgC, char* ArgV[]) -> int override;
    auto init() -> int override;
    auto tick(double deltaTime) -> void override;
    auto tick_rendering(double deltaTime) -> void override;
    auto exit() -> void override;

private:
    auto set_video_play_position(mpv_handle* handle, double playPosition) -> concurrencpp::result<void>;
    auto set_video_duration(mpv_handle* handle, double duration) -> concurrencpp::result<void>;
    auto set_video_end(mpv_handle* handle) -> concurrencpp::result<void>;

    auto wait_for_mpv_events(mpv_handle* handle) -> concurrencpp::result<void>;

    auto get_mpv_handle_from_file(const std::filesystem::path& absoluteFilePath) const -> mpv_handle*;

    struct mpv_context
    {
        mpv_context();
        ~mpv_context();

        mpv_handle* m_mpvHandle                = nullptr;
        mpv_render_context* m_mpvRenderContext = nullptr;

        uint32_t videoTexture = 0;
        uint32_t videoFBO     = 0;

        ImVec2 m_displaySize = ImVec2(1920 / 2, 1080 / 2);
        ImVec2 m_rawVideoSize;

        bool m_playing = false;

        double m_playPosition  = 0.0;
        double m_videoDuration = 0.0;

        std::atomic<bool> m_needsRender;

        concurrencpp::result<void> m_waitEventResult;   //< Storing as a result so we can wait on it at the end to close everything out
    };

    std::unordered_map<mpv_handle*, std::unique_ptr<mpv_context>> m_mpvContexts;
    std::unordered_map<std::string, mpv_handle*> m_videoFileToContexts;

    ma_handle m_mpvLibraryHandle = nullptr;
};
