#pragma once

#include "sound_bakery/core/core_include.h"
#include "sound_bakery/parameter/parameter.h"
#include "sound_bakery/voice/voice.h"

namespace sbk::engine
{
    class container;
    class event;
    class voice;

    class SB_CLASS game_object : public sbk::core::object
    {
        REGISTER_REFLECTION(game_object, sbk::core::object)

    public:
        game_object() = default;

        voice* play_container(container* container);
        void post_event(event* event);

        void stop_voice(voice* voice);
        void stop_container(container* container);
        void stop_all();

        void update();

        [[nodiscard]] bool is_playing() const noexcept;

        /**
         * @brief Finds the parameter value on this gameobject.
         *
         * If there is no local value, the global parameter value is used.
         * @param parameter to get the value for.
         * @return value of the parameter.
         */
        [[nodiscard]] float get_float_parameter_value(const sbk::core::database_ptr<float_parameter>& parameter) const;

        /**
         * @brief Finds the parameter value on this gameobject.
         *
         * If there is no local value, the global parameter value is used.
         * @param parameter to get the value for.
         * @return value of the parameter.
         */
        [[nodiscard]] sbk_id get_int_parameter_value(const sbk::core::database_ptr<named_parameter>& parameter) const;

        void set_float_parameter(const float_parameter::local_parameter_value_pair& parameterValue);
        void set_int_parameter_value(const named_parameter::local_parameter_value_pair& parameterValue);

        [[nodiscard]] local_parameter_list get_local_parameters() const { return m_parameters; }

    private:
        local_parameter_list m_parameters;
    };
}  // namespace sbk::engine