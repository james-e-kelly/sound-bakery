#pragma once

#include "sound_bakery/node/container/container.h"

namespace sbk::engine
{
    class SB_CLASS SwitchContainer : public Container
    {
    public:
        void gatherChildrenForPlay(GatherChildrenContext& context) const override
        {
            sbk::core::DatabasePtr<NamedParameterValue> selectedValue;

            if (auto findLocalValue = context.parameters.intParameters.find(m_switchParameter);
                findLocalValue != context.parameters.intParameters.cend())
            {
                selectedValue = sbk::core::DatabasePtr<NamedParameterValue>(findLocalValue->second.get());
            }
            else if (m_switchParameter.lookup())
            {
                selectedValue = m_switchParameter->getSelectedValue();
            }

            if (auto foundIter = m_switchToChild.find(selectedValue); foundIter != m_switchToChild.end())
            {
                sbk::core::ChildPtr<Container> selectedChild(*this);
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

        void setSwitchParameter(sbk::core::DatabasePtr<NamedParameter> parameter);

        sbk::core::DatabasePtr<NamedParameter> getSwitchParameter() const { return m_switchParameter; }

        std::unordered_map<sbk::core::DatabasePtr<NamedParameterValue>, sbk::core::ChildPtr<Container>>
            getSwitchToChildMap() const
        {
            return m_switchToChild;
        }

    private:
        void setSwitchToChild(
            std::unordered_map<sbk::core::DatabasePtr<NamedParameterValue>, sbk::core::ChildPtr<Container>> map);

        void populateChildKeys();

        /**
         * @brief Pointer to the parameter this container switches upon.
         */
        GlobalIntParameter m_switchParameter;

        /**
         * @brief Holds the map for which switch value maps to which child.
         */
        std::unordered_map<sbk::core::DatabasePtr<NamedParameterValue>, sbk::core::ChildPtr<Container>> m_switchToChild;

        REGISTER_REFLECTION(SwitchContainer, Container)
        RTTR_REGISTRATION_FRIEND
    };
}  // namespace SB::Engine