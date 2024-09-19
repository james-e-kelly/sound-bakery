#pragma once

#include "sound_bakery/node/container/container.h"

namespace sbk::engine
{
    class SB_CLASS switch_container : public container
    {
    public:
        void gather_children_for_play(gather_children_context& context) const override;

        void gatherParametersFromThis(global_parameter_list& parameters) override;

        void setSwitchParameter(sbk::core::database_ptr<named_parameter> parameter);

        sbk::core::database_ptr<named_parameter> getSwitchParameter() const { return m_switchParameter; }

        std::unordered_map<sbk::core::database_ptr<named_parameter_value>, sbk::core::child_ptr<container>>
            getSwitchToChildMap() const
        {
            return m_switchToChild;
        }

    private:
        void setSwitchToChild(
            std::unordered_map<sbk::core::database_ptr<named_parameter_value>, sbk::core::child_ptr<container>> map);

        void populateChildKeys();

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