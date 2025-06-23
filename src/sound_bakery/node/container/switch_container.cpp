#include "switch_container.h"

#include "sound_bakery/parameter/parameter.h"

DEFINE_REFLECTION(sbk::engine::switch_container)

void sbk::engine::switch_container::gather_children_for_play(gather_children_context& context) const
{
    sbk::core::database_ptr<named_parameter_value> selectedValue;

    if (auto findLocalValue = context.parameters.intParameters.find(m_switchParameter);
        findLocalValue != context.parameters.intParameters.cend())
    {
        selectedValue = sbk::core::database_ptr<named_parameter_value>(findLocalValue->second.get());
    }
    else if (m_switchParameter.lookup())
    {
        selectedValue = m_switchParameter->get_selected_value();
    }

    if (auto foundIter = m_switchToChild.find(selectedValue); foundIter != m_switchToChild.end())
    {
        sbk::core::child_ptr<container> selectedChild(*this);
        selectedChild = foundIter->second;

        if (selectedChild.lookup())
        {
            context.sounds.push_back(selectedChild.lookup_raw());
        }
    }
}

void sbk::engine::switch_container::gatherParametersFromThis(global_parameter_list& parameters)
{
    parameters.intParameters.insert(m_switchParameter);
}

void sbk::engine::switch_container::setSwitchParameter(sbk::core::database_ptr<named_parameter> parameter)
{
    m_switchParameter = parameter;

    populateChildKeys();
}

void sbk::engine::switch_container::setSwitchToChild(
    std::unordered_map<sbk::core::database_ptr<named_parameter_value>, sbk::core::child_ptr<container>> map)
{
    if (map.empty())
    {
        populateChildKeys();
    }
    else
    {
        m_switchToChild = map;
    }
}

void sbk::engine::switch_container::populateChildKeys()
{
    m_switchToChild.clear();

    if (m_switchParameter.lookup())
    {
        for (const sbk::core::database_ptr<named_parameter_value>& value : m_switchParameter->get_values())
        {
            m_switchToChild.insert({value, sbk::core::child_ptr<container>(*this)});
        }
    }
}
