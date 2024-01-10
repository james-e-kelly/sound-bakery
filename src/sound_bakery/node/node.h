#pragma once

#include "sound_bakery/core/core_include.h"
#include "sound_bakery/effect/effect.h"

namespace SB::Engine
{
    enum SB_NODE_STATUS
    {
        // Has no parent and no bus
        SB_NODE_NULL,
        // Has a parent node
        SB_NODE_MIDDLE,
        // Has no parent but outputs to a bus
        SB_NODE_TOP
    };

    class NodeBase : public SB::Core::DatabaseObject
    {
    public:
        ~NodeBase();

    public:
        virtual void setParentNode(const SB::Core::DatabasePtr<NodeBase>& parent)
        {
            m_parentNode = parent;
        }

    public:
        SB_NODE_STATUS getNodeStatus() const noexcept
        {
            SB_NODE_STATUS status = SB_NODE_NULL;

            if (m_parentNode.hasId())
            {
                status = SB_NODE_MIDDLE;
            }
            else if (m_outputBus.hasId())
            {
                status = SB_NODE_TOP;
            }

            return status;
        }

        NodeBase* parent() const
        {
            if (m_parentNode.lookup())
            {
                return m_parentNode.raw();
            }
            else
            {
                return nullptr;
            }
        }

        virtual void setOutputBus(const SB::Core::DatabasePtr<NodeBase>& bus)
        {
            m_outputBus = bus;
        }

        NodeBase* outputBus() const
        {
            if (m_outputBus.lookup())
            {
                return m_outputBus.raw();
            }
            else
            {
                return nullptr;
            }
        }

    public:
        virtual bool canAddChild(const SB::Core::DatabasePtr<NodeBase>& child) const
        {
            if (m_childNodes.contains(child))
            {
                return false;
            }
            if (child && child->parent())
            {
                return false;
            }
            return true;
        }

        void addChild(const SB::Core::DatabasePtr<NodeBase>& child)
        {
            if (canAddChild(child))
            {
                m_childNodes.insert(child);

                if (child.lookup())
                {
                    child->setParentNode(this);
                }
            }
        }

        void removeChild(const SB::Core::DatabasePtr<NodeBase>& child)
        {
            if (child)
            {
                child->setParentNode(nullptr);
            }

            m_childNodes.erase(child);
        }

        std::vector<NodeBase*> getChildren() const
        {
            std::vector<NodeBase*> children(m_childNodes.size());

            for (auto& child : m_childNodes)
            {
                if (child.lookup())
                {
                    children.push_back(child.raw());
                }
            }

            return children;
        }

        std::size_t getChildCount() const
        {
            return m_childNodes.size();
        }

    protected:
        SB::Core::DatabasePtr<NodeBase> m_parentNode;
        SB::Core::DatabasePtr<NodeBase> m_outputBus;
        std::unordered_set<SB::Core::DatabasePtr<NodeBase>> m_childNodes;

        RTTR_ENABLE(SB::Core::DatabaseObject)
        RTTR_REGISTRATION_FRIEND
    };

	/**
	 * @brief Root node that builds the core graph of sounds and busses.
	*/
	class Node : public NodeBase
	{
    public:
        SB::Core::FloatProperty m_volume = 1.0f;
        SB::Core::FloatProperty m_pitch = 1.0f;
        SB::Core::FloatProperty m_lowpass = 0.0f;
        SB::Core::FloatProperty m_highpass = 0.0f;

        std::vector<SB::Core::DatabasePtr<EffectDescription>> m_effectDescriptions;

        void addEffect(SC_DSP_TYPE type);
        
        RTTR_ENABLE(NodeBase)
        RTTR_REGISTRATION_FRIEND
	};
}