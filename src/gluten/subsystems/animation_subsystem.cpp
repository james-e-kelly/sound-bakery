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
    m_totalTime += deltaTime;
}

auto gluten::animation_subsystem::exp_smooth(float current, float target, float rate, float dt) -> float
{
    if (rate <= 0.0f || dt <= 0.0f)
    {
        return target;
    }

    const float t = 1.0f - std::exp(-rate * dt);
    return current + (target - current) * t;
}

auto gluten::animation_subsystem::ease_out_cubic(float t) -> float
{
    const float inv = 1.0f - t;
    return 1.0f - inv * inv * inv;
}

auto gluten::animation_subsystem::ease_in_cubic(float t) -> float
{
    return t * t * t;
}

auto gluten::animation_subsystem::clear(ImGuiID id) -> void
{
    if (animation_subsystem* const sub = get())
    {
        sub->m_floatValues.erase(id);
        sub->m_colorValues.erase(id);
    }
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
    const float duration = 3.0f / std::max(rate, 0.01f);

    auto [iter, inserted] = m_floatValues.try_emplace(id, float_anim_state{initial, target, initial, m_totalTime});

    if (inserted)
    {
        return initial;
    }

    if (iter->second.target_value != target)
    {
        iter->second.start_value  = iter->second.current_value;
        iter->second.target_value = target;
        iter->second.start_time   = m_totalTime;
    }

    const float elapsed = static_cast<float>(m_totalTime - iter->second.start_time);
    float t             = std::clamp(elapsed / duration, 0.0f, 1.0f);

    t = ease_out_cubic(t);

    iter->second.current_value = iter->second.start_value + (target - iter->second.start_value) * t;
    return iter->second.current_value;
}

auto gluten::animation_subsystem::animate_color_impl(ImGuiID id, ImVec4 initial, ImVec4 target, float rate) -> ImVec4
{
    const float duration = 3.0f / std::max(rate, 0.01f);

    auto [iter, inserted] = m_colorValues.try_emplace(id, color_anim_state{initial, target, initial, m_totalTime});

    if (inserted)
    {
        return initial;
    }

    if (iter->second.target_value.x != target.x || iter->second.target_value.y != target.y ||
        iter->second.target_value.z != target.z || iter->second.target_value.w != target.w)
    {
        iter->second.start_value  = iter->second.current_value;
        iter->second.target_value = target;
        iter->second.start_time   = m_totalTime;
    }

    const float elapsed = static_cast<float>(m_totalTime - iter->second.start_time);
    const float t       = ease_out_cubic(std::clamp(elapsed / duration, 0.0f, 1.0f));

    iter->second.current_value.x = iter->second.start_value.x + (target.x - iter->second.start_value.x) * t;
    iter->second.current_value.y = iter->second.start_value.y + (target.y - iter->second.start_value.y) * t;
    iter->second.current_value.z = iter->second.start_value.z + (target.z - iter->second.start_value.z) * t;
    iter->second.current_value.w = iter->second.start_value.w + (target.w - iter->second.start_value.w) * t;
    return iter->second.current_value;
}
