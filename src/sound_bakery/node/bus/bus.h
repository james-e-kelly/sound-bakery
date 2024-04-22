#pragma once

#include "sound_bakery/node/node.h"

namespace SB::Engine
{
    class NodeInstance;

    class SB_CLASS Bus : public Node
    {
        REGISTER_REFLECTION(Bus, Node)

    public:
        Bus() : Node(), m_masterBus(false) {}

        void onLoaded() override;

        bool canAddChild(const SB::Core::DatabasePtr<NodeBase>& child) const override
        {
            if (child.lookup() && child->getType().is_derived_from<Bus>())
            {
                return NodeBase::canAddChild(child);
            }
            else
            {
                return false;
            }
        }

        void setMasterBus(bool isMaster)
        {
            if (getType() == rttr::type::get<Bus>())
            {
                m_masterBus = isMaster;
            }
        }

        bool isMasterBus() const
        { 
            return m_masterBus;
        }

    public:
        void lock();
        void unlock();
        std::shared_ptr<NodeInstance> lockAndCopy();

    protected:
        std::shared_ptr<NodeInstance> m_busInstance;

    private:
        bool m_masterBus;
    };
}  // namespace SB::Engine