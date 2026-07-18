#include "gameobject.h"

#include "sound_bakery/voice/voice.h"

using namespace sbk::engine;

DEFINE_REFLECTION(sbk::engine::game_object)

auto game_object::update() -> void
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

auto sbk::engine::game_object::is_playing() const noexcept -> bool { return get_objects_size(); }

auto sbk::engine::game_object::get_float_parameter_value(const sbk::core::database_ptr<float_parameter>& parameter) const -> float
{
    float result = 0.0F;

    auto found = m_parameters.floatParameters.find(parameter);

    if (found != m_parameters.floatParameters.cend())
    {
        result = found->second.get();
    }
    else
    {
        if (const auto parameterShared = parameter.shared())
        {
            result = parameterShared->get();
        }
    }

    return result;
}

auto sbk::engine::game_object::get_int_parameter_value(const sbk::core::database_ptr<named_parameter>& parameter) const -> sbk_id
{
    sbk_id result = 0;

    auto found = m_parameters.intParameters.find(parameter);

    if (found != m_parameters.intParameters.cend())
    {
        result = found->second.get();
    }
    else
    {
        if (const auto parameterShared = parameter.shared())
        {
            result = parameterShared->get();
        }
    }

    return result;
}

auto sbk::engine::game_object::set_float_parameter(const float_parameter::local_parameter_value_pair& parameterValue) -> void
{
    m_parameters.floatParameters[parameterValue.first].set(parameterValue.second);
}

auto sbk::engine::game_object::set_int_parameter_value(const named_parameter::local_parameter_value_pair& parameterValue) -> void
{
    if (m_parameters.intParameters.find(parameterValue.first) == m_parameters.intParameters.cend())
    {
        const sbk::core::database_ptr<named_parameter_value> parameterValuePtr(parameterValue.second);

        if (const auto parameterValuePtrShared = parameterValuePtr.shared())
        {
            if (const auto parentParameter = parameterValuePtrShared->parentParameter.shared())
            {
                m_parameters.intParameters.insert(parentParameter->create_local_parameter_from_this());
            }
        }
    }

    m_parameters.intParameters[parameterValue.first].set(parameterValue.second);

    BOOST_ASSERT(m_parameters.intParameters[parameterValue.first].get() == parameterValue.second);
}
