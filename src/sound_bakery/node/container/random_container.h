#pragma once

#include "sound_bakery/node/container/container.h"

namespace sbk::engine
{
    class SB_CLASS random_container : public container
    {
    public:
        auto gather_children_for_play(gather_children_context& context) const -> void override
        {
            switch (m_childNodes.size())
            {
                case 0:
                    break;
                case 1:
                    if (auto sound = m_childNodes.begin()->shared())
                    {
                        context.sounds.push_back(std::static_pointer_cast<container>(sound));
                    }
                    break;
                default:
                    const int randomChildIndex = std::rand() % m_childNodes.size();
                    auto childIter             = m_childNodes.begin();
                    std::advance(childIter, randomChildIndex);
                    if (auto sound = childIter->shared())
                    {
                        context.sounds.push_back(std::static_pointer_cast<container>(sound));
                    }
                    break;
            }
        }

        REGISTER_REFLECTION(random_container, container)
        RTTR_REGISTRATION_FRIEND
    };
}  // namespace sbk::engine