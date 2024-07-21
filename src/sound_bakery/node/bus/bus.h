#pragma once

#include "sound_bakery/node/node.h"

namespace sbk::engine
{
    class NodeInstance;

    class SB_CLASS bus : public node
    {
        REGISTER_REFLECTION(bus, node)

    public:
        bus() : node(), m_masterBus(false) {}

        bool can_add_child_type(const rttr::type& childType) const override;

        void setMasterBus(bool isMaster);

        bool isMasterBus() const { return m_masterBus; }

        void lock();
        void unlock();
        std::shared_ptr<NodeInstance> lockAndCopy();

    protected:
        std::shared_ptr<NodeInstance> m_busInstance;

    private:
        bool m_masterBus;
    };
}  // namespace sbk::engine