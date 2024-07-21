#include "switch_container.h"

#include "sound_bakery/parameter/parameter.h"

DEFINE_REFLECTION(sbk::engine::SwitchContainer)

void sbk::engine::SwitchContainer::setSwitchParameter(sbk::core::database_ptr<named_parameter> parameter)
{
    m_switchParameter = parameter;

    populateChildKeys();
}

void sbk::engine::SwitchContainer::setSwitchToChild(
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

void sbk::engine::SwitchContainer::populateChildKeys()
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
