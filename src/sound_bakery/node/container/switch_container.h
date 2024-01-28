#pragma once

#include "sound_bakery/node/container/container.h"

namespace SB::Engine
{
    class SwitchContainer : public Container
    {
    public:
        void gatherSounds(GatherSoundsContext& context) override
        {
            if (context.parameters.intParameters.contains(m_switchParameter))
            {
                if (SB::Core::DatabasePtr<NamedParameterValue> selectedValue(
                        context.parameters.intParameters[m_switchParameter].get()); selectedValue.lookup())
                {
                    auto foundIter = m_switchToChild.find(selectedValue);

                    if (foundIter != m_switchToChild.end())
                    {
                        if (foundIter->second.lookup())
                        {
                            foundIter->second->gatherSounds(context);
                        }
                    }
                }
            }
            else
            {
                if (const NamedParameter* const switchParameter = m_switchParameter.raw())
                {
                    if (const SB::Core::DatabasePtr<NamedParameterValue> selectedValue = switchParameter->getSelectedValue();
                        selectedValue.lookup())
                    {
                        auto foundIter = m_switchToChild.find(selectedValue);

                        if (foundIter != m_switchToChild.end())
                        {
                            if (foundIter->second.lookup())
                            {
                                foundIter->second->gatherSounds(context);
                            }
                        }
                    }
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