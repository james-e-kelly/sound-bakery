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
     * @brief Root node that builds the core graph of sounds and busses.
     */
    class SB_CLASS node : public sbk::core::database_object
    {
    public:
        ~node();

        sbk::core::float_property m_volume   = sbk::core::float_property(1.0f, 0.0f, 1.0f);
        sbk::core::float_property m_pitch    = sbk::core::float_property(1.0f, 0.0f, 2.0f);
        sbk::core::float_property m_lowpass  = sbk::core::float_property(1.0f, 0.0f, 100.0f);
        sbk::core::float_property m_highpass = sbk::core::float_property(1.0f, 0.0f, 100.0f);

        eastl::vector<sbk::core::database_ptr<effect_description>> m_effectDescriptions;

        virtual auto set_output_bus(const sbk::core::database_ptr<node>& bus) -> void;

        [[nodiscard]] auto get_parent() const -> std::shared_ptr<node>;
        [[nodiscard]] auto get_output_bus() const -> std::shared_ptr<node>;

        [[nodiscard]] auto get_node_status() const noexcept -> node_status;

        [[nodiscard]] virtual auto can_add_children() const -> bool;
        [[nodiscard]] virtual auto can_add_child_type(const rttr::type& childType) const -> bool;
        [[nodiscard]] auto can_add_child(const sbk::core::database_ptr<node>& child) const -> bool;

        [[nodiscard]] virtual auto can_add_parent() const -> bool;
        [[nodiscard]] virtual auto can_add_parent_type(const rttr::type& parentType) const -> bool;

        auto add_child(const sbk::core::database_ptr<node>& child) -> void;
        auto remove_child(const sbk::core::database_ptr<node>& child) -> void;

        [[nodiscard]] auto get_children() const -> eastl::vector<std::shared_ptr<node>>;
        [[nodiscard]] auto get_child_count() const -> std::size_t;
        [[nodiscard]] auto has_child(const sbk::core::database_ptr<node>& test) const -> bool;

        auto gather_all_descendants(eastl::vector<std::shared_ptr<node>>& descendants) const -> void;
        auto gather_all_parents(eastl::vector<std::shared_ptr<node>>& parents) const -> void;

        virtual auto gather_parameters(global_parameter_list& parameters) -> void;

        auto add_effect(sc_dsp_type type) -> sbk::result<void>;
        auto add_effect_clap(const clap_plugin_factory_t* clapFactory) -> sbk::result<void>;

    protected:
        virtual auto set_parent_node(const sbk::core::database_ptr<node>& parent) -> void;

        virtual auto gather_parameters_from_this(global_parameter_list& parameters) -> void { (void)parameters; }

        sbk::core::database_ptr<node> m_parentNode;
        sbk::core::database_ptr<node> m_outputBus;
        std::unordered_set<sbk::core::database_ptr<node>> m_childNodes;

        REGISTER_REFLECTION(node, sbk::core::database_object)
    };

    using node_base = node;
}  // namespace sbk::engine
