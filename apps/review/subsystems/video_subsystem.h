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

    auto load_video(const std::filesystem::path& absoluteFilePath) const -> void;
    auto get_video_texture() const -> uint32_t;

protected:
    auto pre_init(int ArgC, char* ArgV[]) -> int override;
    auto init() -> int override;
    auto tick(double deltaTime) -> void override;
    auto tick_rendering(double deltaTime) -> void override;
    auto exit() -> void override;

private:
    ma_handle m_mpvLibraryHandle = nullptr;

    mpv_handle* m_mpvHandle = nullptr;
    mpv_render_context* m_mpvRenderContext = nullptr;

    
};
