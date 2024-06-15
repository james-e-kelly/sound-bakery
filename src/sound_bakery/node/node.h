#pragma once

#include "sound_bakery/core/core_include.h"
#include "sound_bakery/effect/effect.h"
#include "sound_bakery/parameter/parameter.h"

namespace sbk::engine
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

    class SB_CLASS NodeBase : public sbk::core::database_object
    {
    public:
        ~NodeBase();

    public:
        virtual void setParentNode(const sbk::core::DatabasePtr<NodeBase>& parent);
        virtual void setOutputBus(const sbk::core::DatabasePtr<NodeBase>& bus);

        SB_NODE_STATUS getNodeStatus() const noexcept;

        NodeBase* parent() const;
        NodeBase* outputBus() const;

        virtual bool canAddChild(const sbk::core::DatabasePtr<NodeBase>& child) const;

        void addChild(const sbk::core::DatabasePtr<NodeBase>& child);
        void removeChild(const sbk::core::DatabasePtr<NodeBase>& child);

        std::vector<NodeBase*> getChildren() const;
        std::size_t getChildCount() const;
        bool hasChild(const sbk::core::DatabasePtr<NodeBase>& test) const;

        void gatherAllDescendants(std::vector<NodeBase*>& descendants) const;
        void gatherAllParents(std::vector<NodeBase*>& parents) const;

    protected:
        sbk::core::DatabasePtr<NodeBase> m_parentNode;
        sbk::core::DatabasePtr<NodeBase> m_outputBus;
        std::unordered_set<sbk::core::DatabasePtr<NodeBase>> m_childNodes;

        REGISTER_REFLECTION(NodeBase, sbk::core::database_object)
    };

    /**
     * @brief Root node that builds the core graph of sounds and busses.
     */
    class SB_CLASS Node : public NodeBase
    {
    public:
        sbk::core::FloatProperty m_volume   = sbk::core::FloatProperty(1.0f, 0.0f, 1.0f);
        sbk::core::FloatProperty m_pitch    = sbk::core::FloatProperty(1.0f, 0.0f, 1.0f);
        sbk::core::FloatProperty m_lowpass  = sbk::core::FloatProperty(1.0f, 0.0f, 100.0f);
        sbk::core::FloatProperty m_highpass = sbk::core::FloatProperty(1.0f, 0.0f, 100.0f);

        std::vector<sbk::core::DatabasePtr<EffectDescription>> m_effectDescriptions;

        /**
         * @brief Gathers all parameters on this and child nodes that can effect the runtime output.
         */
        virtual void gatherParameters(GlobalParameterList& parameters);

        void addEffect(sc_dsp_type type);

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