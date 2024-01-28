#pragma once

#include "sound_bakery/node/container/container.h"

namespace SB::Engine
{
    class SwitchContainer : public Container
    {
    public:
        void gatherSounds(GatherSoundsContext& context) override
        {
            SB::Core::DatabasePtr<NamedParameterValue> selectedValue;

            if (auto findLocalValue = context.parameters.intParameters.find(m_switchParameter);
                findLocalValue != context.parameters.intParameters.cend())
            {
                selectedValue = SB::Core::DatabasePtr<NamedParameterValue>(findLocalValue->second.get());
            }
            else if (m_switchParameter.lookup())
            {
                selectedValue = m_switchParameter->getSelectedValue();
            }

            if (auto foundIter = m_switchToChild.find(selectedValue); foundIter != m_switchToChild.end())
            {
                SB::Core::ChildPtr<Container> selectedChild(*this);
                selectedChild = foundIter->second;

                if (selectedChild.lookup())
                {
                    selectedChild->gatherSounds(context);
                }
            }
        }

        void gatherParametersFromThis(GlobalParameterList& parameters) override
        {
            parameters.intParameters.insert(m_switchParameter);
        }

        void setSwitchParameter(SB::Core::DatabasePtr<NamedParameter> parameter);

        SB::Core::DatabasePtr<NamedParameter> getSwitchParameter() const { return m_switchParameter; }

        std::unordered_map<SB::Core::DatabasePtr<NamedParameterValue>, SB::Core::ChildPtr<Container>>
            getSwitchToChildMap() const
        {
            return m_switchToChild;
        }

    private:
        void setSwitchToChild(
            std::unordered_map<SB::Core::DatabasePtr<NamedParameterValue>, SB::Core::ChildPtr<Container>> map);

        void populateChildKeys();

        /**
         * @brief Pointer to the parameter this container switches upon.
         */
        GlobalIntParameter m_switchParameter;

        /**
         * @brief Holds the map for which switch value maps to which child.
         */
        std::unordered_map<SB::Core::DatabasePtr<NamedParameterValue>, SB::Core::ChildPtr<Container>> m_switchToChild;

        RTTR_ENABLE(Container)
        RTTR_REGISTRATION_FRIEND
    };
}  // namespace SB::Engine