#include "animation_subsystem.h"

#include "gluten/app/app.h"
#include "gluten/theme/theme.h"

auto gluten::animation_subsystem::get() -> animation_subsystem*
{
    if (app* const owningApp = app::get())
    {
        return owningApp->get_subsystem_by_class<animation_subsystem>().get();
    }
    return nullptr;
}

auto gluten::animation_subsystem::pre_tick(double deltaTime) -> void
{
    m_deltaTime = deltaTime;
}

auto gluten::animation_subsystem::exp_smooth(float current, float target, float rate, float dt) -> float
{
    if (rate <= 0.0f || dt <= 0.0f)
    {
        return target;
    }

    // 1 - e^(-rate * dt) is the frame-rate-independent lerp factor for an
    // exponential decay. At 60 FPS with rate=15 this is ~22 % per frame.
    const float t = 1.0f - std::exp(-rate * dt);
    return current + (target - current) * t;
}

auto gluten::animation_subsystem::animate(ImGuiID id, float initial, float target, float rate) -> float
{
    if (animation_subsystem* const sub = get())
    {
        return sub->animate_impl(id, initial, target, rate);
    }
    return initial;
}

auto gluten::animation_subsystem::animate_bool(ImGuiID id, bool initialState, bool state, float rate) -> float
{
    return animate(id, initialState ? 1.0F : 0.0F, state ? 1.0f : 0.0f, rate);
}

auto gluten::animation_subsystem::animate_color(ImGuiID id, ImVec4 initial, ImVec4 target, float rate) -> ImVec4
{
    if (animation_subsystem* const sub = get())
    {
        return sub->animate_color_impl(id, initial, target, rate);
    }
    return initial;
}

auto gluten::animation_subsystem::animate_color(ImGuiID id, ImU32 initial, ImU32 target, float rate) -> ImU32
{
    const ImVec4 animated  = animate_color(id, ImGui::ColorConvertU32ToFloat4(initial), ImGui::ColorConvertU32ToFloat4(target), rate);
    return ImGui::ColorConvertFloat4ToU32(animated);
}

auto gluten::animation_subsystem::animate_impl(ImGuiID id, float initial, float target, float rate) -> float
{
    const auto [iter, inserted] = m_floatValues.try_emplace(id, initial);
    if (inserted)
    {
        return initial;
    }

    const float dt = static_cast<float>(m_deltaTime);
    iter->second   = exp_smooth(iter->second, target, rate, dt);
    return iter->second;
}

auto gluten::animation_subsystem::animate_color_impl(ImGuiID id, ImVec4 initial, ImVec4 target, float rate) -> ImVec4
{
    const auto [iter, inserted] = m_colorValues.try_emplace(id, initial);
    if (inserted)
    {
        return initial;
    }

    const float dt = static_cast<float>(m_deltaTime);
    iter->second.x = exp_smooth(iter->second.x, target.x, rate, dt);
    iter->second.y = exp_smooth(iter->second.y, target.y, rate, dt);
    iter->second.z = exp_smooth(iter->second.z, target.z, rate, dt);
    iter->second.w = exp_smooth(iter->second.w, target.w, rate, dt);
    return iter->second;
}
