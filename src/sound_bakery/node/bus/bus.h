#pragma once

#include "sound_bakery/node/node.h"

namespace sbk::engine
{
    class node_instance;

    class SB_CLASS bus : public node
    {
        REGISTER_REFLECTION(bus, node)

    public:
        bus() : node(), m_masterBus(false) {}

        bool can_add_child_type(const rttr::type& childType) const override;

        bool can_add_parent() const override;
        bool can_add_parent_type(const rttr::type& parentType) const override;

        void setMasterBus(bool isMaster);

        bool isMasterBus() const { return m_masterBus; }

        void lock();
        void unlock();
        std::shared_ptr<node_instance> lockAndCopy();

    protected:
        std::shared_ptr<node_instance> m_busInstance;

    private:
        bool m_masterBus;
    };
}  // namespace sbk::engine