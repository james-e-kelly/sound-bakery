#pragma once

#include "sound_bakery/core/core_include.h"
#include "sound_bakery/effect/effect.h"
#include "sound_bakery/parameter/parameter.h"

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

    class SB_CLASS NodeBase : public SB::Core::DatabaseObject
    {
    public:
        ~NodeBase();

    public:
        virtual void setParentNode(const SB::Core::DatabasePtr<NodeBase>& parent);
        virtual void setOutputBus(const SB::Core::DatabasePtr<NodeBase>& bus);

        SB_NODE_STATUS getNodeStatus() const noexcept;

        NodeBase* parent() const;
        NodeBase* outputBus() const;

        virtual bool canAddChild(const SB::Core::DatabasePtr<NodeBase>& child) const;

        void addChild(const SB::Core::DatabasePtr<NodeBase>& child);
        void removeChild(const SB::Core::DatabasePtr<NodeBase>& child);

        std::vector<NodeBase*> getChildren() const;
        std::size_t getChildCount() const;
        bool hasChild(const SB::Core::DatabasePtr<NodeBase>& test) const;

        void gatherAllDescendants(std::vector<NodeBase*>& descendants) const;
        void gatherAllParents(std::vector<NodeBase*>& parents) const;

    protected:
        SB::Core::DatabasePtr<NodeBase> m_parentNode;
        SB::Core::DatabasePtr<NodeBase> m_outputBus;
        std::unordered_set<SB::Core::DatabasePtr<NodeBase>> m_childNodes;

        REGISTER_REFLECTION(NodeBase, SB::Core::DatabaseObject)
    };

    /**
     * @brief Root node that builds the core graph of sounds and busses.
     */
    class SB_CLASS Node : public NodeBase
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
        virtual void gatherParameters(GlobalParameterList& parameters);

        void addEffect(SC_DSP_TYPE type);

    protected:
        /**
         * @brief Appends parameters from this node that are relevant to the runtime output.
         *
         * Called from gatherParameters.
         * @param parameters to append to.
         */
        virtual void gatherParametersFromThis(GlobalParameterList& parameters) { (void)parameters; }

        REGISTER_REFLECTION(Node, NodeBase)
    };
}  // namespace SB::Engine