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

        bool canAddChild(const sbk::core::database_ptr<node_base>& child) const override
        {
            if (child.lookup() && child->getType().is_derived_from<bus>())
            {
                return node_base::canAddChild(child);
            }
            else
            {
                return false;
            }
        }

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