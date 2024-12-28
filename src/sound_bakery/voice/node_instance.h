#pragma once

#include "sound_bakery/core/core_include.h"

#include "sound_bakery/node/container/sequence_container.h"

#include <boost/msm/back/state_machine.hpp>
#include <boost/msm/front/state_machine_def.hpp>
#include <boost/msm/front/functor_row.hpp>
#include <boost/msm/front/euml/common.hpp>
#include <boost/msm/front/euml/operator.hpp>
#include <boost/msm/front/euml/state_grammar.hpp>

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
    
    struct flag_playing{};
    struct flag_stopped{};

    struct SB_CLASS node_instance_fsm : public boost::msm::front::state_machine_def<node_instance_fsm>
    {
        ~node_instance_fsm();


        struct state_uninit : public boost::msm::front::state<>{};
        struct state_init : public boost::msm::front::state<>{};
        struct state_playing : public boost::msm::front::state<>
        {
            typedef boost::mpl::vector1<flag_playing> flag_list;
        };
        struct state_virtual : public boost::msm::front::state<>
        {
            typedef boost::mpl::vector1<flag_playing> flag_list;
        };
        struct state_stopped : public boost::msm::front::state<>
        {
            typedef boost::mpl::vector1<flag_stopped> flag_list;
        };

        auto action_init(const event_init& init) -> void;
        auto action_play(const event_play& play) -> void;
        auto action_pause(const event_pause& pause) -> void;
        auto action_stop(const event_stop& stop) -> void;
        auto action_virtualise(const event_virtualise& virtualise) -> void;
        auto action_devirtualise(const event_devirtualise& devirtualise) -> void;
        struct SB_CLASS action_update
        {
            template <class event_class, class state_machine_class, class source_state_class, class target_state_class>
            void operator()(event_class const& update, state_machine_class& stateMachine, source_state_class&, target_state_class&)
            {
                if (stateMachine.m_soundInstance)
                {
                    if (ma_sound_at_end(&stateMachine.m_soundInstance->sound) == MA_TRUE)
                    {
                        stateMachine.process_event(event_stop());
                    }
                }
                else if (!stateMachine.m_children.empty())
                {
                    unsigned int stoppedChildren = 0;

                    for (const auto& child : stateMachine.m_children)
                    {
                        child->update();

                        if (!child->is_playing())
                        {
                            ++stoppedChildren;
                        }
                    }

                    const bool allChildrenHaveStopped = stoppedChildren == stateMachine.m_children.size();

                    if (allChildrenHaveStopped)
                    {
                        // Sequence nodes retrigger when the current sound stops
                        if (stateMachine.m_referencingNode->get_object_type() == rttr::type::get<sbk::engine::sequence_container>())
                        {
                            stateMachine.m_children.clear();
                            ++stateMachine.m_numTimesPlayed;
                            stateMachine.init_child();

                            stateMachine.process_event(event_play());
                        }
                        else
                        {
                            stateMachine.process_event(event_stop());
                        }
                    }
                }
            }
        };

        auto guard_init(const event_init& init) -> bool;

        typedef state_uninit initial_state;  // the initial state of the player SM

        struct transition_table : boost::mpl::vector
            <
                  // Start, Event, Next, Action, Guard
                  row<state_uninit, event_init, state_init, &node_instance_fsm::action_init, &node_instance_fsm::guard_init>,
                  a_row<state_init, event_play, state_playing, &node_instance_fsm::action_play>,
                  a_row<state_playing, event_stop, state_stopped, &node_instance_fsm::action_stop>,
                  a_row<state_playing, event_virtualise, state_virtual, &node_instance_fsm::action_virtualise>,
                  a_row<state_virtual, event_devirtualise, state_playing, &node_instance_fsm::action_devirtualise>,
                  boost::msm::front::Row<state_playing, event_update, boost::msm::front::none, action_update, boost::msm::front::none>
            >
        {};

        void init_parent();
        void init_child();
        auto init_node_group(const event_init& init) -> sb_result;
        void init_callbacks();

        static auto add_dsp_to_node_group(sc_node_group* nodeGroup, sc_dsp** dsp, const sc_dsp_config& config)
            -> sb_result;

        auto set_volume(float oldVolume, float newVolume) -> void;
        auto set_pitch(float oldPitch, float newPitch) -> void;
        auto set_lowpass(float oldLowpass, float newLowpass) -> void;
        auto set_highpass(float oldHighpass, float newHighpass) -> void;

        std::shared_ptr<node> m_referencingNode;
        class sbk::engine::node_instance* m_owner = nullptr;
        node_group_instance m_nodeGroup;
        std::shared_ptr<node_instance> m_parent;
        std::vector<std::shared_ptr<node_instance>> m_children;
        std::unique_ptr<sc_sound_instance, SC_SOUND_INSTANCE_DELETER> m_soundInstance;
        unsigned int m_numTimesPlayed = 0;
    };

    /**
     * @brief NodeInstances represent runtime versions of Nodes, either
     * containers or busses
     */
    class SB_CLASS node_instance : public sbk::core::object, public boost::msm::front::state_machine_def<node_instance>
    {
        REGISTER_REFLECTION(node_instance, sbk::core::object)

    public:
        auto init(const event_init& init) -> sb_result;
        auto play() -> sb_result;
        auto stop(float fadeTime = 0.0f) -> sb_result;
        auto update() -> sb_result;

        auto is_playing() const -> bool;
        auto is_stopped() const -> bool;

        auto get_referencing_node() const noexcept -> std::shared_ptr<node>;
        auto get_parent() const noexcept -> node_instance*;
        auto get_bus() const noexcept -> sc_node_group*;

    private:
        boost::msm::back::state_machine<node_instance_fsm> m_stateMachine;
    };
}  // namespace sbk::engine
