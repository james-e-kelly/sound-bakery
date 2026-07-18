#pragma once

#include "sound_bakery/node/container/container.h"

namespace sbk::engine
{
    class SB_CLASS random_container : public container
    {
    public:
        virtual auto gather_children_for_play(gather_children_context& context) const -> void override
        {
            switch (m_childNodes.size())
            {
                case 0:
                    break;
                case 1:
                    context.sounds.push_back(m_childNodes.begin()->lookup_raw()->try_convert_object<container>());
                    break;
                default:
                    int randomChildIndex = std::rand() % m_childNodes.size();
                    std::unordered_set<sbk::core::database_ptr<node_base>>::const_iterator childIter = m_childNodes.begin();
                    std::advance(childIter, randomChildIndex);
                    if (childIter->lookup())
                    {
                        context.sounds.push_back(childIter->raw()->try_convert_object<container>());
                    }
                    break;
            }
        }

        REGISTER_REFLECTION(random_container, container)
        RTTR_REGISTRATION_FRIEND
    };
}  // namespace sbk::engine