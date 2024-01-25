#include "switch_container.h"

#include "sound_bakery/parameter/parameter.h"

void SB::Engine::SwitchContainer::setSwitchParameter(
    SB::Core::DatabasePtr<IntParameter> parameter)
{
    m_switchParameter = parameter;

    populateChildKeys();
}

void SB::Engine::SwitchContainer::setSwitchToChild(
    std::unordered_map<SB::Core::DatabasePtr<IntParameterValue>,
                       SB::Core::ChildPtr<Container>> map)
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

void SB::Engine::SwitchContainer::populateChildKeys()
{
    m_switchToChild.clear();

    if (m_switchParameter.lookup())
    {
        for (const SB::Core::DatabasePtr<IntParameterValue>& value :
             m_switchParameter->getValues())
        {
            m_switchToChild.insert({value, SB::Core::ChildPtr<Container>(*this)});
        }
    }
}
