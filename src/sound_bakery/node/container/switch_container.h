#pragma once

#include "sound_bakery/node/container/container.h"

namespace SB::Engine
{
    class IntParameter;
    class IntParameterValue;

    class SwitchContainer : public Container
    {
    public:
        virtual void gatherSounds(std::vector<Container*>& soundContainers, const RuntimeFloatParameterMap& runtimeFloatParameters, const RuntimeIntParameterMap& runtimeIntParameters) override
        {
            if (runtimeIntParameters.contains(m_switchParameter.id()))
            {

            }
            else
            {
                if (IntParameter* switchParameter = m_switchParameter.raw())
                {
                    if (SB::Core::DatabasePtr<IntParameterValue> selectedValue = switchParameter->getSelectedValue(); selectedValue.lookup())
                    {
                        auto foundIter = m_switchToChild.find(selectedValue);

                        if (foundIter != m_switchToChild.end())
                        {
                            if (foundIter->second.lookup())
                            {
                                foundIter->second->gatherSounds(soundContainers, runtimeFloatParameters, runtimeIntParameters);
                            }
                        }
                    }
                }
            }
        }

    public:
        void setSwitchParameter(SB::Core::DatabasePtr<IntParameter> parameter);

        SB::Core::DatabasePtr<IntParameter> getSwitchParameter() const
        {
            return m_switchParameter;
        }

        std::unordered_map<SB::Core::DatabasePtr<IntParameterValue>, SB::Core::DatabasePtr<Container>> getSwitchToChildMap() const
        {
            return m_switchToChild;
        }

    private:
        void setSwitchToChild(std::unordered_map<SB::Core::DatabasePtr<IntParameterValue>, SB::Core::DatabasePtr<Container>> map);

    private:
        void populateChildKeys();

        /**
         * @brief Pointer to the parameter this container switches upon.
        */
        SB::Core::DatabasePtr<IntParameter> m_switchParameter;

        /**
         * @brief Holds the map for which switch value maps to which child.
        */
        std::unordered_map<SB::Core::DatabasePtr<IntParameterValue>, SB::Core::DatabasePtr<Container>> m_switchToChild;

        RTTR_ENABLE(Container)
        RTTR_REGISTRATION_FRIEND
    };
}