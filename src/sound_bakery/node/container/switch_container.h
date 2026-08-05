#pragma once

#include "sound_bakery/core/database/database_ptr.h"
#include "sound_bakery/node/container/container.h"

namespace sbk::engine
{
    class SB_CLASS switch_container : public container
    {
    public:
        auto gather_children_for_play(gather_children_context& context) const -> void override;

        auto gather_parameters_from_this(global_parameter_list& parameters) -> void override;

        auto set_switch_parameter(sbk::core::database_ptr<named_parameter> parameter) -> void;

        [[nodiscard]] auto get_switch_parameters() const -> sbk::core::database_ptr<named_parameter> { return m_switchParameter; }

        [[nodiscard]] auto get_switch_to_child_map() const
            -> std::unordered_map<sbk::core::database_ptr<named_parameter_value>, sbk::core::child_ptr<container>>
        {
            return m_switchToChild;
        }

    private:
        auto set_switch_to_child(
            std::unordered_map<sbk::core::database_ptr<named_parameter_value>, sbk::core::child_ptr<container>> map)
            -> void;

        auto populate_child_keys() -> void;

        /**
         * @brief Pointer to the parameter this container switches upon.
         */
        global_int_parameter m_switchParameter;

        /**
         * @brief Holds the map for which switch value maps to which child.
         */
        std::unordered_map<sbk::core::database_ptr<named_parameter_value>, sbk::core::child_ptr<container>>
            m_switchToChild;

        REGISTER_REFLECTION(switch_container, container)
        RTTR_REGISTRATION_FRIEND
    };
}  // namespace sbk::engine