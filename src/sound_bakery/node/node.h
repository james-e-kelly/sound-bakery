#pragma once

#include "sound_bakery/core/core_include.h"
#include "sound_bakery/effect/effect.h"
#include "sound_bakery/parameter/parameter.h"

namespace SB::Engine
{
    class FloatParameter;
    class NamedParameter;
    class NamedParameterValue;

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
        virtual void setParentNode(const SB::Core::DatabasePtr<NodeBase>& parent) { m_parentNode = parent; }

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

        virtual void setOutputBus(const SB::Core::DatabasePtr<NodeBase>& bus) { m_outputBus = bus; }

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
            if (m_childNodes.contains(child) || child.id() == getDatabaseID())
            {
                return false;
            }
            return true;
        }

        void addChild(const SB::Core::DatabasePtr<NodeBase>& child)
        {
            if (canAddChild(child))
            {
                if (child.lookup() && child->parent())
                {
                    child->parent()->removeChild(child);
                }

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

        std::size_t getChildCount() const { return m_childNodes.size(); }

        bool hasChild(const SB::Core::DatabasePtr<NodeBase>& test) const { return m_childNodes.contains(test); }

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
        SB::Core::FloatProperty m_volume   = SB::Core::FloatProperty(1.0f, 0.0f, 1.0f);
        SB::Core::FloatProperty m_pitch    = SB::Core::FloatProperty(1.0f, 0.0f, 1.0f);
        SB::Core::FloatProperty m_lowpass  = SB::Core::FloatProperty(1.0f, 0.0f, 100.0f);
        SB::Core::FloatProperty m_highpass = SB::Core::FloatProperty(1.0f, 0.0f, 100.0f);

        std::vector<SB::Core::DatabasePtr<EffectDescription>> m_effectDescriptions;

        /**
         * @brief Gathers all parameters on this and child nodes that can effect the runtime output.
         */
        virtual void gatherParameters(GlobalParameterList& parameters)
        {
            parameters.floatParameters.reserve(m_childNodes.size() + 1);
            parameters.intParameters.reserve(m_childNodes.size() + 1);

            gatherParametersFromThis(parameters);

            for (NodeBase* const child : getChildren())
            {
                if (child != nullptr)
                {
                    if (Node* const childNode = child->tryConvertObject<Node>())
                    {
                        childNode->gatherParameters(parameters);
                    }
                }
            }
        }

        void addEffect(SC_DSP_TYPE type);

    protected:
        /**
         * @brief Appends parameters from this node that are relevant to the runtime output.
         *
         * Called from gatherParameters.
         * @param parameters to append to.
         */
        virtual void gatherParametersFromThis(GlobalParameterList& parameters) { (void)parameters; }

        RTTR_ENABLE(NodeBase)
        RTTR_REGISTRATION_FRIEND
    };
}  // namespace SB::Engine