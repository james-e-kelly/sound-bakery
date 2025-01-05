#include "gameobject.h"

#include "sound_bakery/event/event.h"
#include "sound_bakery/node/container/container.h"
#include "sound_bakery/voice/voice.h"

using namespace sbk::engine;

DEFINE_REFLECTION(sbk::engine::game_object)

voice* game_object::play_container(container* container)
{
    if (container)
    {
        if (std::shared_ptr<voice> voice = create_runtime_object<sbk::engine::voice>())
        {
            voice->play_container(container);
            return voice.get();
        }
    }
    return nullptr;
}

void sbk::engine::game_object::post_event(event* event)
{
    ZoneScoped;

    if (event)
    {
        SBK_INFO("Posting Event");

        for (const action& action : event->m_actions)
        {
            sbk::engine::container* container    = nullptr;
            sbk::engine::event* childEvent       = nullptr;
            sbk::engine::game_object* gameObject = nullptr;

            if (const sbk::core::database_ptr<sbk::core::database_object>& destination = action.m_destination;
                destination.lookup())
            {
                container  = destination->try_convert_object<sbk::engine::container>();
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
    ZoneScoped;
    for (auto iter = get_objects().begin(); iter != get_objects().end(); ++iter)
    {
        if (iter->get() == voice)
        {
            remove_object(*iter);
            break;
        }
    }
}

void sbk::engine::game_object::stop_container(container* container)
{
    ZoneScoped;
    for (auto iter = get_objects().begin(); iter != get_objects().end(); ++iter)
    {
        if (const sbk::engine::voice* const voice = iter->get()->try_convert_object<sbk::engine::voice>())
        {
            if (voice->playing_container(container))
            {
                remove_object(*iter);
                break;
            }
        }
    }
}

void game_object::stop_all() 
{ 
    ZoneScoped;
    destroy_all();   //< Assuming we only own voices 
}

void game_object::update()
{
    ZoneScoped;
    for (auto iter = get_objects().begin(); iter != get_objects().end();)
    {
        if (sbk::engine::voice* voice = iter->get()->try_convert_object<sbk::engine::voice>())
        {
            voice->update();

            if (!voice->is_playing())
            {
                iter = remove_object(*iter);
            }
            else
            {
                ++iter;
            }
        }
    }
}

bool sbk::engine::game_object::is_playing() const noexcept { return get_objects_size(); }

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

    BOOST_ASSERT(m_parameters.intParameters[parameterValue.first].get() == parameterValue.second);
}
