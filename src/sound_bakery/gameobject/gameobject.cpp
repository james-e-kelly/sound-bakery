#include "gameobject.h"

#include "sound_bakery/event/event.h"
#include "sound_bakery/node/container/container.h"
#include "sound_bakery/voice/voice.h"

using namespace sbk::engine;

DEFINE_REFLECTION(sbk::engine::game_object)

auto game_object::play_container(container* container, const pass_key<sbk::engine::system>& passkey) -> voice*
{
    if (container)
    {
        if (const std::shared_ptr<voice> voice = create_runtime_object<sbk::engine::voice>().get())
        {
            voice->play_container(container);
            return voice.get();
        }
    }
    return nullptr;
}

auto sbk::engine::game_object::post_event(event* event, const pass_key<sbk::engine::system>& passkey) -> void
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
                destination.lookup().get())
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
                        play_container(container, passkey);
                    }
                    else if (childEvent)
                    {
                        post_event(childEvent, passkey);
                    }
                    break;
                case SB_ACTION_STOP:
                    if (container)
                    {
                        stop_container(container, passkey);
                    }
                    else if (childEvent)
                    {
                    }
                    else if (gameObject)
                    {
                        gameObject->stop_all(passkey);
                    }
                    break;
                default:
                    break;
            }
        }
    }
}

void sbk::engine::game_object::stop_voice(voice* voice, const pass_key<sbk::engine::system>& passkey)
{
    ZoneScoped;
    
    iterate_referenced_objects([voice](std::shared_ptr<object> object)
        {
            if (object.get() == voice)
            {
                return sbk::core::object_iterate_action::destroy_and_stop;
            }
            return sbk::core::object_iterate_action::next;
        });
}

void sbk::engine::game_object::stop_container(container* container, const pass_key<sbk::engine::system>& passkey)
{
    ZoneScoped;

    iterate_referenced_objects([container](std::shared_ptr<object> object)
        {
            if (const sbk::engine::voice* const voice = object->try_convert_object<sbk::engine::voice>())
            {
                if (voice->playing_container(container))
                {
                    return sbk::core::object_iterate_action::destroy_and_stop;
                }
            }

            return sbk::core::object_iterate_action::next;
        });
}

void game_object::stop_all(const pass_key<sbk::engine::system>& passkey)
{ 
    ZoneScoped;
    remove_all();   //< Assuming we only own voices 
}

void game_object::update()
{
    ZoneScoped;

    iterate_referenced_objects([](std::shared_ptr<object> object)
        {
            if (sbk::engine::voice* voice = object->try_convert_object<sbk::engine::voice>())
            {
                voice->update();

                if (!voice->is_playing())
                {
                    return sbk::core::object_iterate_action::destroy_and_next;
                }
            }
            return sbk::core::object_iterate_action::next;
        });
}

bool sbk::engine::game_object::is_playing() const noexcept { return get_referenced_objects_size().get(); }

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
        if (parameter.lookup().get())
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
        if (parameter.lookup().get())
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
        parameterValuePtr.lookup().get();
        parameterValuePtr->parentParameter.lookup().get();

        m_parameters.intParameters.insert(parameterValuePtr->parentParameter->create_local_parameter_from_this());
    }

    m_parameters.intParameters[parameterValue.first].set(parameterValue.second);

    BOOST_ASSERT(m_parameters.intParameters[parameterValue.first].get() == parameterValue.second);
}
