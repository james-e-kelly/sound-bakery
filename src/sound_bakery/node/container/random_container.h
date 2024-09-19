#pragma once

#include "sound_bakery/node/container/container.h"

namespace sbk::engine
{
    class SB_CLASS RandomContainer : public container
    {
    public:
        virtual void gather_children_for_play(gather_children_context& context) const override
        {
            switch (m_childNodes.size())
            {
                case 0:
                    break;
                case 1:
                    context.sounds.push_back(m_childNodes.begin()->lookupRaw()->try_convert_object<container>());
                    break;
                default:
                    int randomChildIndex = std::rand() % m_childNodes.size();
                    std::unordered_set<sbk::core::database_ptr<node_base>>::const_iterator childIter =
                        m_childNodes.begin();
                    std::advance(childIter, randomChildIndex);
                    context.sounds.push_back(childIter->lookupRaw()->try_convert_object<container>());
                    break;
            }
        }

        REGISTER_REFLECTION(RandomContainer, container)
        RTTR_REGISTRATION_FRIEND
    };
}  // namespace sbk::engine