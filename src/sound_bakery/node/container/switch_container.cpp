#include "switch_container.h"

#include "sound_bakery/parameter/parameter.h"

DEFINE_REFLECTION(sbk::engine::SwitchContainer)

void sbk::engine::SwitchContainer::setSwitchParameter(sbk::core::database_ptr<NamedParameter> parameter)
{
    m_switchParameter = parameter;

    populateChildKeys();
}

void sbk::engine::SwitchContainer::setSwitchToChild(
    std::unordered_map<sbk::core::database_ptr<NamedParameterValue>, sbk::core::child_ptr<Container>> map)
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
        for (const sbk::core::database_ptr<NamedParameterValue>& value : m_switchParameter->getValues())
        {
            m_switchToChild.insert({value, sbk::core::child_ptr<Container>(*this)});
        }
    }
}
