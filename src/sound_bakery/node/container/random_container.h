#pragma once

#include "sound_bakery/node/container/container.h"

namespace SB::Engine
{
    class RandomContainer : public Container
    {
    public:

        virtual void gatherSounds(std::vector<Container*>& soundContainers) override
        {
            switch (m_childNodes.size())
            {
            case 0:
                break;
            case 1:
                dynamic_cast<Container*>(m_childNodes.begin()->raw())->gatherSounds(soundContainers);
                break;
            default:
                int randomChildIndex = std::rand() % m_childNodes.size();
                std::unordered_set<SB::Core::DatabasePtr<NodeBase>>::iterator childIter = m_childNodes.begin();
                std::advance(childIter, randomChildIndex);
                dynamic_cast<Container*>(childIter->raw())->gatherSounds(soundContainers);
                break;
            }
        }

        RTTR_ENABLE(Container)
        RTTR_REGISTRATION_FRIEND
    };
}