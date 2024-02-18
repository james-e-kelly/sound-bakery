#pragma once

#include "sound_bakery/node/container/container.h"

namespace SB::Engine
{
    class SequenceContainer : public Container
    {
    public:
        virtual void gatherChildrenForPlay(GatherChildrenContext& context) const override;

    private:
        std::vector<SB::Core::ChildPtr<Container>> m_sequence;

        RTTR_ENABLE(Container)
        RTTR_REGISTRATION_FRIEND
    };
}  // namespace SB::Engine