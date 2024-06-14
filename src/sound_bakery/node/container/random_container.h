#pragma once

#include "sound_bakery/node/container/container.h"

namespace SB::Engine
{
    class SB_CLASS RandomContainer : public Container
    {
    public:
        virtual void gatherChildrenForPlay(GatherChildrenContext& context) const override
        {
            switch (m_childNodes.size())
            {
                case 0:
                    break;
                case 1:
                    context.sounds.push_back(m_childNodes.begin()->lookupRaw()->try_convert_object<Container>());
                    break;
                default:
                    int randomChildIndex = std::rand() % m_childNodes.size();
                    std::unordered_set<SB::Core::DatabasePtr<NodeBase>>::const_iterator childIter =
                        m_childNodes.begin();
                    std::advance(childIter, randomChildIndex);
                    context.sounds.push_back(childIter->lookupRaw()->try_convert_object<Container>());
                    break;
            }
        }

        REGISTER_REFLECTION(RandomContainer, Container)
        RTTR_REGISTRATION_FRIEND
    };
}  // namespace SB::Engine