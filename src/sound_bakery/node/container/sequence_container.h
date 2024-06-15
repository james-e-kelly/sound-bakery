#pragma once

#include "sound_bakery/node/container/container.h"

namespace sbk::engine
{
    class SB_CLASS SequenceContainer : public Container
    {
    public:
        virtual void gatherChildrenForPlay(GatherChildrenContext& context) const override;

    private:
        std::vector<sbk::core::ChildPtr<Container>> m_sequence;

        REGISTER_REFLECTION(SequenceContainer, Container)
        RTTR_REGISTRATION_FRIEND
    };
}  // namespace SB::Engine