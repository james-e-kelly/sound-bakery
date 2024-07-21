#include "gameobject.h"

#include "sound_bakery/event/event.h"
#include "sound_bakery/node/container/container.h"
#include "sound_bakery/voice/voice.h"

using namespace sbk::engine;

DEFINE_REFLECTION(sbk::engine::game_object)

voice* game_object::play_container(Container* container)
{
    if (container)
    {
        std::unique_ptr<voice>& voice = m_voices.emplace_back(std::make_unique<sbk::engine::voice>(this));
        voice->playContainer(container);
        return voice.get();
    }
    else
    {
        return nullptr;
    }
}

void sbk::engine::game_object::post_event(event* event)
{
    if (event)
    {
        for (const action& action : event->m_actions)
        {
            sbk::engine::Container* container   = nullptr;
            sbk::engine::event* childEvent      = nullptr;
            sbk::engine::game_object* gameObject = nullptr;

            if (const sbk::core::database_ptr<sbk::core::database_object>& destination = action.m_destination;
                destination.lookup())
            {
                container  = destination->try_convert_object<sbk::engine::Container>();
                childEvent = destination->try_convert_object<sbk::engine::event>();
                gameObject = destination->try_convert_object<sbk::engine::game_object>();
            }

            switch (action.m_type)
            {
                case SB_ACTION_PLAY:
                    if (container)
                    {
                        play_container(container);
                    }
                    else if (childEvent)
                    {
                        post_event(childEvent);
                    }
                    break;
                case SB_ACTION_STOP:
                    if (container)
                    {
                        stop_container(container);
                    }
                    else if (childEvent)
                    {
                    }
                    else if (gameObject)
                    {
                        gameObject->stop_all();
                    }
                    break;
                default:
                    break;
            }
        }
    }
}

void sbk::engine::game_object::stop_voice(voice* voice)
{
    for (std::size_t i = m_voices.size(); i--;)
    {
        if (m_voices[i].get() == voice)
        {
            m_voices.erase(m_voices.begin() + i);
            break;
        }
    }
}

void sbk::engine::game_object::stop_container(Container* container)
{
    for (std::size_t i = m_voices.size(); i--;)
    {
        if (const std::unique_ptr<voice>& voice = m_voices[i])
        {
            if (voice->playingContainer(container))
            {
                m_voices.erase(m_voices.begin() + i);
                break;
            }
        }
    }
}

void game_object::stop_all() { m_voices.clear(); }

void game_object::update()
{
    if (m_voices.size())
    {
        std::vector<std::unique_ptr<voice>>::iterator iter;
        for (iter = m_voices.begin(); iter != m_voices.end();)
        {
            iter->get()->update();

            if (!iter->get()->isPlaying())
            {
                iter = m_voices.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
    }
}

bool sbk::engine::game_object::is_playing() const noexcept { return voice_count(); }

std::size_t sbk::engine::game_object::voice_count() const noexcept { return m_voices.size(); }

voice* sbk::engine::game_object::get_voice(std::size_t index) const
{
    if (index >= 0 && index < m_voices.size())
    {
        return m_voices[index].get();
    }
    return nullptr;
}

float sbk::engine::game_object::get_float_parameter_value(
    const sbk::core::database_ptr<float_parameter>& parameter) const
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

sbk_id sbk::engine::game_object::get_int_parameter_value(
    const sbk::core::database_ptr<named_parameter>& parameter) const
{
    sbk_id result = 0;

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

void sbk::engine::game_object::set_float_parameter(const float_parameter::local_parameter_value_pair& parameterValue)
{
    m_parameters.floatParameters[parameterValue.first].set(parameterValue.second);
}

void sbk::engine::game_object::set_int_parameter_value(
    const named_parameter::local_parameter_value_pair& parameterValue)
{
    if (m_parameters.intParameters.find(parameterValue.first) == m_parameters.intParameters.cend())
    {
        const sbk::core::database_ptr<named_parameter_value> parameterValuePtr(parameterValue.second);
        parameterValuePtr.lookup();
        parameterValuePtr->parentParameter.lookup();

        m_parameters.intParameters.insert(parameterValuePtr->parentParameter->create_local_parameter_from_this());
    }

    m_parameters.intParameters[parameterValue.first].set(parameterValue.second);

    assert(m_parameters.intParameters[parameterValue.first].get() == parameterValue.second);
}
