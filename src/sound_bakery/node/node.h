#pragma once

#include "sound_bakery/core/database/database_object.h"
#include "sound_bakery/effect/effect.h"
#include "sound_bakery/parameter/parameter.h"

namespace sbk::engine
{
    enum class node_status
    {
        // Has no get_parent and no bus
        null,
        // Has a get_parent node
        middle,
        // Has no get_parent but outputs to a bus
        top
    };

    /**
     * @brief Generic node that can have a get_parent and own children.
     */
    class SB_CLASS node_base : public sbk::core::database_object
    {
    public:
        ~node_base();

        virtual auto set_output_bus(const sbk::core::database_ptr<node_base>& bus) -> void;

        [[nodiscard]] auto get_parent() const -> std::shared_ptr<node_base>;
        [[nodiscard]] auto get_output_bus() const -> std::shared_ptr<node_base>;

        [[nodiscard]] auto get_node_status() const noexcept -> node_status;

        [[nodiscard]] virtual auto can_add_children() const -> bool;                                      //< Can any children be added to this node?
        [[nodiscard]] virtual auto can_add_child_type(const rttr::type& childType) const -> bool;         //< Can this type be added to the child?
        [[nodiscard]] auto can_add_child(const sbk::core::database_ptr<node_base>& child) const -> bool;  //< Can this runtime child be added?

        [[nodiscard]] virtual auto can_add_parent() const -> bool;                                   //< Can any parents be added to this node?
        [[nodiscard]] virtual auto can_add_parent_type(const rttr::type& parentType) const -> bool;  //< Can this type be added as a
                                                                                                     // get_parent?

        auto add_child(const sbk::core::database_ptr<node_base>& child) -> void;
        auto remove_child(const sbk::core::database_ptr<node_base>& child) -> void;

        [[nodiscard]] auto get_children() const -> eastl::vector<std::shared_ptr<node_base>>;
        [[nodiscard]] auto get_child_count() const -> std::size_t;
        [[nodiscard]] auto has_child(const sbk::core::database_ptr<node_base>& test) const -> bool;

        auto gather_all_descendants(eastl::vector<std::shared_ptr<node_base>>& descendants) const -> void;
        auto gather_all_parents(eastl::vector<std::shared_ptr<node_base>>& parents) const -> void;

    protected:
        /**
         * @brief Set the parent node. Does not add this node to the parent's child list.
         */
        virtual auto set_parent_node(const sbk::core::database_ptr<node_base>& parent) -> void;

        sbk::core::database_ptr<node_base> m_parentNode;
        sbk::core::database_ptr<node_base> m_outputBus;
        std::unordered_set<sbk::core::database_ptr<node_base>> m_childNodes;

    private:
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

        eastl::vector<sbk::core::database_ptr<effect_description>> m_effectDescriptions;

        /**
         * @brief Gathers all parameters on this and child nodes that can effect the runtime output.
         */
        virtual auto gather_parameters(global_parameter_list& parameters) -> void;

        auto add_effect(sc_dsp_type type) -> sbk::result<void>;
        auto add_effect_clap(const clap_plugin_factory_t* clapFactory) -> sbk::result<void>;

    protected:
        /**
         * @brief Appends parameters from this node that are relevant to the runtime output.
         *
         * Called from gather_parameters.
         * @param parameters to append to.
         */
        virtual auto gather_parameters_from_this(global_parameter_list& parameters) -> void { (void)parameters; }

        REGISTER_REFLECTION(node, node_base)
    };
}  // namespace sbk::engine