#include "profiler_widget.h"

#include "gluten/app/app.h"
#include "gluten/theme/theme.h"
#include "gluten/utils/imgui_util_structures.h"

#include "sound_bakery/system.h"
#include "TracyView.hpp"
#include "Fonts.hpp"

ImTextureID zigzagTex;

namespace profiler_utils
{
    static constexpr std::size_t stringBufferSize       = 512;
    static char connectionAddress[stringBufferSize]     = { "127.0.0.1\0"};
    static constexpr unsigned short connectionPort      = 8086;
}

static FontData g_fonts;
static float FontNormal = 16.0f;
static float FontSmall  = 14.0f;
static float FontBig    = 18.0f;

static auto tracy_mainthread_callback(const std::function<void()>& function, bool forceDelay) -> void 
{
    sbk::engine::system::get()->get_game_thread_executer()->post(function);
}

static auto tracy_set_title(const char* title) -> void
{

}

static auto tracy_set_scale(float scale) -> void
{

}

static auto tracy_attention() -> void
{

}

auto profiler_widget::start_implementation() -> void
{
    g_fonts.normal = get_app()->get_font(gluten::fonts::regular);
    g_fonts.bold   = get_app()->get_font(gluten::fonts::title);
    g_fonts.italic = get_app()->get_font(gluten::fonts::light);
    g_fonts.mono   = get_app()->get_font(gluten::fonts::regular);
    g_fonts.boldItalic = get_app()->get_font(gluten::fonts::title);
}

auto profiler_widget::render_implementation() -> void
{
    gluten::imgui::scoped_font fontAwesome(get_app()->get_font(gluten::fonts::regular_font_awesome));

    if (m_tracy)
    {
        m_tracy->Draw();
    }
    else
    {
        ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Appearing);
        ImGui::Begin(get_widget_name().data());

        ImGui::TextUnformatted("Client Address");
        bool connectClicked = ImGui::InputTextWithHint("###connectionAddress", "Enter address", profiler_utils::connectionAddress,
                                    profiler_utils::stringBufferSize, ImGuiInputTextFlags_EnterReturnsTrue);
        connectClicked |= ImGui::Button(ICON_FA_WIFI  " Connect");

        if (connectClicked)
        {
            m_tracy = std::make_unique<tracy::View>(
                tracy_mainthread_callback, profiler_utils::connectionAddress, profiler_utils::connectionPort,
                get_app()->get_font(gluten::fonts::regular), 
                get_app()->get_font(gluten::fonts::light),
                get_app()->get_font(gluten::fonts::title), 
                tracy_set_title, 
                tracy_set_scale, 
                tracy_attention, 
                m_tracyConfig, nullptr);
        }
        ImGui::End();
    }
}

auto profiler_widget::end_implementation() -> void
{
    m_tracy.reset();
}