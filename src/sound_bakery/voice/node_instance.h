#pragma once

#include "sound_bakery/core/core_include.h"

#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>

namespace sbk::engine
{
    class bus;
    class container;
    class node;
    class node_base;
    class node_instance;
    class sound;
    class sound_container;
    class voice;

    /**
     * @brief Owns a node group and applies DSP effects to it.
     */
    struct SB_CLASS node_group_instance
    {
        sc_dsp* lowpass  = nullptr;
        sc_dsp* highpass = nullptr;
        std::unique_ptr<sc_node_group, SC_NODE_GROUP_DELETER> nodeGroup;
    };

    enum class node_instance_type
    {
        child,
        bus,
        main
    };

    struct event_init
    {
        sbk::core::database_ptr<node_base> refNode;
        node_instance_type type          = node_instance_type::main;
        node_instance* parentForChildren = nullptr;
    };
    struct event_play
    {
    };
    struct event_pause
    {
    };
    struct event_stop
    {
        float stopTime = 0.0f;
    };
    struct event_virtualise
    {
    };
    struct event_devirtualise
    {
    };
    struct event_update
    {
        float deltaTime = 0.0f;
    };

    /**
     * @brief NodeInstances represent runtime versions of Nodes, either
     * containers or busses
     */
    class SB_CLASS node_instance : public sbk::core::object, public boost::msm::front::state_machine_def<node_instance>
    {
        REGISTER_REFLECTION(node_instance, sbk::core::object)
    public:
        ~node_instance();

        auto init(const event_init& init) -> sb_result;
        auto play() -> sb_result;
        auto stop(float fadeTime = 0.0f) -> sb_result;

        void update();

        auto is_playing() const -> bool;
        auto is_stopped() const -> bool;

        auto get_referencing_node() const noexcept -> std::shared_ptr<node>;
        auto get_parent() const noexcept -> node_instance*;
        auto get_bus() const noexcept -> sc_node_group*;

    private:
        struct state_uninit : public boost::msm::front::state<>{};
        struct state_init : public boost::msm::front::state<>{};
        struct state_playing : public boost::msm::front::state<>{};
        struct state_stopped : public boost::msm::front::state<>{};
        struct state_virtual : public boost::msm::front::state<>{};

        auto action_init(const event_init& init) -> void;
        auto action_play(const event_play& play) -> void;
        auto action_pause(const event_pause& pause) -> void;
        auto action_stop(const event_stop& stop) -> void;
        auto action_virtualise(const event_virtualise& virtualise) -> void;
        auto action_devirtualise(const event_devirtualise& devirtualise) -> void;
        auto action_update(const event_update& update) -> void;

        auto guard_init(const event_init& init) -> bool;

        typedef state_uninit initial_state; // the initial state of the player SM

        // Start, Event, Next, Action, Guard
        struct transition_table : boost::mpl::vector
            <
                row<state_uninit, event_init, state_init, &node_instance::action_init, &node_instance::guard_init>,
                a_row<state_init, event_play, state_playing, &node_instance::action_play>,
                a_row<state_playing, event_stop, state_stopped, &node_instance::action_stop>,
                a_row<state_playing, event_virtualise, state_virtual, &node_instance::action_virtualise>,
                a_row<state_virtual, event_devirtualise, state_playing, &node_instance::action_devirtualise>,
                a_irow<state_playing, event_update, &node_instance::action_update>
            >
        {};

        void init_parent();
        void init_child();
        auto init_node_group(const event_init& init) -> sb_result;
        void init_callbacks();

        static auto add_dsp_to_node_group(sc_node_group* nodeGroup, sc_dsp** dsp, const sc_dsp_config& config) -> sb_result;

        auto set_volume(float oldVolume, float newVolume) -> void;
        auto set_pitch(float oldPitch, float newPitch) -> void;
        auto set_lowpass(float oldLowpass, float newLowpass) -> void;
        auto set_highpass(float oldHighpass, float newHighpass) -> void;

        std::shared_ptr<node> m_referencingNode;
        node_group_instance m_nodeGroup;
        std::shared_ptr<node_instance> m_parent;
        std::vector<std::shared_ptr<node_instance>> m_children;
        std::unique_ptr<sc_sound_instance, SC_SOUND_INSTANCE_DELETER> m_soundInstance;
        unsigned int m_numTimesPlayed = 0;

        //boost::msm::back::state_machine<node_instance> m_stateMachine;
    };
}  // namespace sbk::engine
