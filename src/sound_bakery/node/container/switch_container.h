#pragma once

#include "sound_bakery/node/container/container.h"

namespace sbk::engine
{
    class SB_CLASS SwitchContainer : public Container
    {
    public:
        void gatherChildrenForPlay(GatherChildrenContext& context) const override
        {
            sbk::core::database_ptr<named_parameter_value> selectedValue;

            if (auto findLocalValue = context.parameters.intParameters.find(m_switchParameter);
                findLocalValue != context.parameters.intParameters.cend())
            {
                selectedValue = sbk::core::database_ptr<named_parameter_value>(findLocalValue->second.get());
            }
            else if (m_switchParameter.lookup())
            {
                selectedValue = m_switchParameter->get_selected_value();
            }

            if (auto foundIter = m_switchToChild.find(selectedValue); foundIter != m_switchToChild.end())
            {
                sbk::core::child_ptr<Container> selectedChild(*this);
                selectedChild = foundIter->second;

                if (selectedChild.lookup())
                {
                    context.sounds.push_back(selectedChild.lookupRaw());
                }
            }
        }

        void gatherParametersFromThis(global_parameter_list& parameters) override
        {
            parameters.intParameters.insert(m_switchParameter);
        }

        void setSwitchParameter(sbk::core::database_ptr<named_parameter> parameter);

        sbk::core::database_ptr<named_parameter> getSwitchParameter() const { return m_switchParameter; }

        std::unordered_map<sbk::core::database_ptr<named_parameter_value>, sbk::core::child_ptr<Container>>
            getSwitchToChildMap() const
        {
            return m_switchToChild;
        }

    private:
        void setSwitchToChild(
            std::unordered_map<sbk::core::database_ptr<named_parameter_value>, sbk::core::child_ptr<Container>> map);

        void populateChildKeys();

        /**
         * @brief Pointer to the parameter this container switches upon.
         */
        global_int_parameter m_switchParameter;

        /**
         * @brief Holds the map for which switch value maps to which child.
         */
        std::unordered_map<sbk::core::database_ptr<named_parameter_value>, sbk::core::child_ptr<Container>>
            m_switchToChild;

        REGISTER_REFLECTION(SwitchContainer, Container)
        RTTR_REGISTRATION_FRIEND
    };
}  // namespace sbk::engine