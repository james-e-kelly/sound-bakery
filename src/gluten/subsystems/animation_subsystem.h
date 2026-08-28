#pragma once

#include "gluten/subsystems/subsystem.h"

#include "imgui.h"

#include <unordered_map>

namespace gluten
{
    /**
     * @brief Home for every animation primitive in gluten.
     *
     * Owns per-frame delta time (captured in pre_tick) and the state for
     * every id-keyed animated float or colour in the app. Elements call the
     * static wrappers each frame — the subsystem stores the current value
     * keyed by @p id, eases it toward @p target using the app's own delta
     * time, and returns the new value. First call for a given id snaps to
     * initial so nothing eases in from zero.
     *
     * Animations use duration-based easing curves rather than exponential
     * decay. Direction is detected automatically: moving toward a higher
     * value uses ease-out (responsive), moving toward a lower value uses
     * ease-in (gentle departure).
     *
     * Auto-registered by app::run() so animations "just work". To disable
     * animations globally, don't register the subsystem — every static
     * wrapper below then snaps to @p target with no easing, and no elements
     * change behaviour.
     */
    class animation_subsystem : public subsystem
    {
    public:
        using subsystem::subsystem;

        static auto get() -> animation_subsystem*;

        // ---- Public animation API — safe when the subsystem is absent -----

        /**
         * @brief Ease a float value toward @p target across frames.
         *
         * @param id      Stable identifier for the value. Typically
         *                ImGui::GetID(name).
         * @param initial Value on the very first call for this id.
         * @param target  Value to ease toward.
         * @param rate    Controls speed — mapped to a duration of ~3/rate
         *                seconds. 12–20 feels snappy; 4–8 is lazy.
         * @return Current value; @p initial if the subsystem isn't registered.
         */
        static auto animate(ImGuiID id, float initial, float target, float rate = 15.0f) -> float;

        /**
         * @brief Convenience — animate a boolean state as a 0–1 float.
         */
        static auto animate_bool(ImGuiID id, bool initialState, bool state, float rate = 15.0f) -> float;

        /**
         * @brief Ease an RGBA colour toward @p target across frames.
         */
        static auto animate_color(ImGuiID id, ImU32 initial, ImU32 target, float rate = 20.0f) -> ImU32;
        static auto animate_color(ImGuiID id, ImVec4 initial, ImVec4 target, float rate = 20.0f) -> ImVec4;

        /**
         * @brief Remove all stored state for @p id so the next animate()
         *        call treats it as a first-ever call and snaps to initial.
         */
        static auto clear(ImGuiID id) -> void;

        /**
         * @brief Pure frame-rate-independent exponential smoothing helper.
         *
         * Stateless. Use it directly when you already own the current value
         * and just need a step. Not used internally any more but kept as a
         * utility for one-off smoothing outside the animation system.
         */
        static auto exp_smooth(float current, float target, float rate, float dt) -> float;

        // ---- Subsystem lifecycle ------------------------------------------

        auto pre_tick(double deltaTime) -> void override;

        auto get_delta_time() const -> float { return static_cast<float>(m_deltaTime); }

    private:
        struct float_anim_state
        {
            float start_value;
            float target_value;
            float current_value;
            double start_time;
        };

        struct color_anim_state
        {
            ImVec4 start_value;
            ImVec4 target_value;
            ImVec4 current_value;
            double start_time;
        };

        static auto ease_out_cubic(float t) -> float;
        static auto ease_in_cubic(float t) -> float;

        auto animate_impl(ImGuiID id, float initial, float target, float rate) -> float;
        auto animate_color_impl(ImGuiID id, ImVec4 initial, ImVec4 target, float rate) -> ImVec4;

        double m_deltaTime  = 0.0;
        double m_totalTime  = 0.0;
        std::unordered_map<ImGuiID, float_anim_state> m_floatValues;
        std::unordered_map<ImGuiID, color_anim_state> m_colorValues;
    };
}  // namespace gluten
