#pragma once

#include "sound_bakery/core/core_include.h"
#include "sound_bakery/parameter/parameter.h"
#include "sound_bakery/voice/voice.h"

namespace SB::Engine
{
    class Container;
    class Event;

    class GameObject : public SB::Core::Object
    {
    public:
        Voice* playContainer(Container* container);
        void postEvent(Event* event);

        void stopVoice(Voice* voice);
        void stopContainer(Container* container);
        void stopAll();

        void update();

        bool isPlaying() const noexcept;

        std::size_t voiceCount() const noexcept;

        Voice* getVoice(std::size_t index) const;

        /**
         * @brief Finds the parameter value on this gameobject.
         *
         * If there is no local value, the global parameter value is used.
         * @param parameter to get the value for.
         * @return value of the parameter.
         */
        float getFloatParameterValue(const SB::Core::DatabasePtr<FloatParameter>& parameter) const
        {
            float result = 0.0F;

            auto found = m_parameters.floatParameters.find(parameter);

            if (found != m_parameters.floatParameters.cend())
            {
                result = found->second.get();
            }
            else
            {
                if (parameter.lookup())
                {
                    result = parameter.raw()->get();
                }
            }

            return result;
        }

        /**
         * @brief Finds the parameter value on this gameobject.
         *
         * If there is no local value, the global parameter value is used.
         * @param parameter to get the value for.
         * @return value of the parameter.
         */
        SB_ID getIntParameterValue(const SB::Core::DatabasePtr<NamedParameter>& parameter) const
        {
            SB_ID result = 0;

            auto found = m_parameters.intParameters.find(parameter);

            if (found != m_parameters.intParameters.cend())
            {
                result = found->second.get();
            }
            else
            {
                if (parameter.lookup())
                {
                    result = parameter.raw()->get();
                }
            }

            return result;
        }

        void setFloatParameterValue(const FloatParameter::LocalParameterValuePair& parameterValue)
        {
            m_parameters.floatParameters[parameterValue.first].set(parameterValue.second);
        }

        void setIntParameterValue(const NamedParameter::LocalParameterValuePair& parameterValue)
        {
            if (m_parameters.intParameters.find(parameterValue.first) == m_parameters.intParameters.cend())
            {
                SB::Core::DatabasePtr<NamedParameterValue> parameterValuePtr(parameterValue.second);
                parameterValuePtr.lookup();
                parameterValuePtr->parentParameter.lookup();

                m_parameters.intParameters.insert(parameterValuePtr->parentParameter->createLocalParameterFromThis());
            }

            m_parameters.intParameters[parameterValue.first].set(parameterValue.second);

            assert(m_parameters.intParameters[parameterValue.first].get() == parameterValue.second);
        }

        LocalParameterList getLocalParameters() const { return m_parameters; }

    private:
        std::vector<std::unique_ptr<Voice>> m_voices;
        LocalParameterList m_parameters;
    };
}  // namespace SB::Engine