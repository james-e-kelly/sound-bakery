#pragma once

#include "sound_bakery/node/container/container.h"

namespace sbk::engine
{
    class SB_CLASS SwitchContainer : public Container
    {
    public:
        void gatherChildrenForPlay(GatherChildrenContext& context) const override
        {
            sbk::core::database_ptr<NamedParameterValue> selectedValue;

            if (auto findLocalValue = context.parameters.intParameters.find(m_switchParameter);
                findLocalValue != context.parameters.intParameters.cend())
            {
                selectedValue = sbk::core::database_ptr<NamedParameterValue>(findLocalValue->second.get());
            }
            else if (m_switchParameter.lookup())
            {
                selectedValue = m_switchParameter->getSelectedValue();
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

        void gatherParametersFromThis(GlobalParameterList& parameters) override
        {
            parameters.intParameters.insert(m_switchParameter);
        }

        void setSwitchParameter(sbk::core::database_ptr<NamedParameter> parameter);

        sbk::core::database_ptr<NamedParameter> getSwitchParameter() const { return m_switchParameter; }

        std::unordered_map<sbk::core::database_ptr<NamedParameterValue>, sbk::core::child_ptr<Container>>
            getSwitchToChildMap() const
        {
            return m_switchToChild;
        }

    private:
        void setSwitchToChild(
            std::unordered_map<sbk::core::database_ptr<NamedParameterValue>, sbk::core::child_ptr<Container>> map);

        void populateChildKeys();

        /**
         * @brief Pointer to the parameter this container switches upon.
         */
        GlobalIntParameter m_switchParameter;

        /**
         * @brief Holds the map for which switch value maps to which child.
         */
        std::unordered_map<sbk::core::database_ptr<NamedParameterValue>, sbk::core::child_ptr<Container>>
            m_switchToChild;

        REGISTER_REFLECTION(SwitchContainer, Container)
        RTTR_REGISTRATION_FRIEND
    };
}  // namespace sbk::engine