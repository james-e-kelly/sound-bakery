#include "switch_container.h"

#include "sound_bakery/parameter/parameter.h"

DEFINE_REFLECTION(sbk::engine::switch_container)

auto sbk::engine::switch_container::gather_children_for_play(gather_children_context& context) const -> void
{
    ZoneScoped;
    sbk::core::database_ptr<named_parameter_value> selectedValue;

    if (auto findLocalValue = context.parameters.intParameters.find(m_switchParameter); findLocalValue != context.parameters.intParameters.cend())
    {
        selectedValue = sbk::core::database_ptr<named_parameter_value>(findLocalValue->second.get());
    }
    else if (auto switchParameter = m_switchParameter.shared())
    {
        selectedValue = switchParameter->get_selected_value();
    }

    if (auto foundIter = m_switchToChild.find(selectedValue); foundIter != m_switchToChild.end())
    {
        sbk::core::child_ptr<container> selectedChild(*this);
        selectedChild = foundIter->second;

        if (auto selectedChildShared = selectedChild.shared())
        {
            context.sounds.push_back(selectedChildShared);
        }
    }
}

auto sbk::engine::switch_container::gather_parameters_from_this(global_parameter_list& parameters) -> void
{
    parameters.intParameters.insert(m_switchParameter);
}

auto sbk::engine::switch_container::set_switch_parameter(sbk::core::database_ptr<named_parameter> parameter) -> void
{
    m_switchParameter = parameter;

    populate_child_keys();
}

auto sbk::engine::switch_container::set_switch_to_child(std::unordered_map<sbk::core::database_ptr<named_parameter_value>, sbk::core::child_ptr<container>> map) -> void
{
    if (map.empty())
    {
        populate_child_keys();
    }
    else
    {
        m_switchToChild = map;
    }
}

auto sbk::engine::switch_container::populate_child_keys() -> void
{
    m_switchToChild.clear();

    if (auto switchParameter = m_switchParameter.shared())
    {
        for (const sbk::core::database_ptr<named_parameter_value>& value : switchParameter->get_values())
        {
            m_switchToChild.insert({value, sbk::core::child_ptr<container>(*this)});
        }
    }
}
