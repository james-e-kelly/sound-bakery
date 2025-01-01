#include "profiler_widget.h"

#include "sound_bakery/system.h"
#include "TracyView.hpp"

ImTextureID zigzagTex;

static auto tracy_mainthread_callback(const std::function<void()>& function, bool forceDelay) -> void 
{
    sbk::engine::system::get()->get_game_thread_executer()->post(function);
}

auto profiler_widget::start_implementation() -> void
{

}

auto profiler_widget::render_implementation() -> void
{
    if (m_tracy)
    {
        m_tracy->Draw();
    }
}

auto profiler_widget::end_implementation() -> void
{
    m_tracy.reset();
}