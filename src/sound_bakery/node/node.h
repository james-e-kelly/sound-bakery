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

    class SB_CLASS node_base : public sbk::core::database_object
    {
    public:
        ~node_base();

    public:
        virtual void setParentNode(const sbk::core::database_ptr<node_base>& parent);
        virtual void setOutputBus(const sbk::core::database_ptr<node_base>& bus);

        SB_NODE_STATUS getNodeStatus() const noexcept;

        node_base* parent() const;
        node_base* outputBus() const;

        virtual bool can_add_children() const;  //< Can any children be added to this node?
        virtual bool can_add_child_type(const rttr::type& childType) const; //< Can this type be added to the child?
        bool can_add_child(const sbk::core::database_ptr<node_base>& child) const;  //< Can this runtime child be added?

        virtual bool can_add_parent() const;    //< Can any parents be added to this node?
        virtual bool can_add_parent_type(const rttr::type& parentType) const;   //< Can this type be added as a parent?

        void addChild(const sbk::core::database_ptr<node_base>& child);
        void removeChild(const sbk::core::database_ptr<node_base>& child);

        std::vector<node_base*> getChildren() const;
        std::size_t getChildCount() const;
        bool hasChild(const sbk::core::database_ptr<node_base>& test) const;

        void gatherAllDescendants(std::vector<node_base*>& descendants) const;
        void gatherAllParents(std::vector<node_base*>& parents) const;

    protected:
        sbk::core::database_ptr<node_base> m_parentNode;
        sbk::core::database_ptr<node_base> m_outputBus;
        std::unordered_set<sbk::core::database_ptr<node_base>> m_childNodes;

        REGISTER_REFLECTION(node_base, sbk::core::database_object)
    };

    /**
     * @brief Root node that builds the core graph of sounds and busses.
     */
    class SB_CLASS node : public node_base
    {
    public:
        sbk::core::float_property m_volume   = sbk::core::float_property(1.0f, 0.0f, 1.0f);
        sbk::core::float_property m_pitch    = sbk::core::float_property(1.0f, 0.0f, 1.0f);
        sbk::core::float_property m_lowpass  = sbk::core::float_property(1.0f, 0.0f, 100.0f);
        sbk::core::float_property m_highpass = sbk::core::float_property(1.0f, 0.0f, 100.0f);

        std::vector<sbk::core::database_ptr<effect_description>> m_effectDescriptions;

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

        REGISTER_REFLECTION(node, node_base)
    };
}  // namespace sbk::engine