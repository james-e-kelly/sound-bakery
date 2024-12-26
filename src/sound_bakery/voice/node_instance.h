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
        bool init_node_group(const node_base& node);

        sc_dsp* lowpass  = nullptr;
        sc_dsp* highpass = nullptr;
        std::unique_ptr<sc_node_group, SC_NODE_GROUP_DELETER> nodeGroup;
    };

    /**
     * @brief Owns a get_parent node instance.
     */
    struct SB_CLASS parent_node_owner
    {
        bool create_parent(const node_base& thisNode, voice* owningVoice);

        std::shared_ptr<node_instance> parent;
    };

    /**
     * @brief Owns a list of child node instances.
     */
    struct SB_CLASS children_node_owner
    {
        bool createChildren(const node_base& thisNode,
                            voice* owningVoice,
                            node_instance* thisNodeInstance,
                            unsigned int numTimesPlayed);

        std::vector<std::shared_ptr<node_instance>> childrenNodes;
    };

    enum class NodeInstanceType
    {
        CHILD,
        BUS,
        MAIN
    };

    enum class node_instance_state
    {
        UNINIT,

        IDLE,       //< Initialized but not started
        STOPPED,    //< Stopped/finished
        STOPPING,   //<
        PAUSED,     //< Explicitly paused and awaiting a start
        VIRTUAL,    //< Not processing but playing

        PLAYING     //< Actively playing
    };

    struct SB_CLASS init_node_instance
    {
        /**
         * @brief Node to reference
         */
        sbk::core::database_ptr<node_base> refNode;

        /**
         * @brief Type of node to create.
         *
         * Different types of nodes initialize differently. For example, get_parent nodes only create more parents.
         * Children create more children.
         */
        NodeInstanceType type = NodeInstanceType::MAIN;

        /**
         * @brief voice owner.
         */
        voice* owningVoice = nullptr;

        /**
         * @brief Parent node instance for this node instance.
         *
         * Used when initializing children so it can join the DSP graph correctly.
         */
        node_instance* parentForChildren = nullptr;
    };

    struct event_init {};
    struct event_play {};
    struct event_pause {};
    struct event_stop { float stopTime = 0.0f; };
    struct event_virtualise{};
    struct event_devirtualise {};

    /**
     * @brief NodeInstances represent runtime versions of Nodes, either
     * containers or busses
     */
    class SB_CLASS node_instance : public sbk::core::object, public boost::msm::front::state_machine_def<node_instance>
    {
    public:
        ~node_instance();

        struct state_uninit : public boost::msm::front::state<>{};
        struct state_init : public boost::msm::front::state<>{};
        struct state_playing : public boost::msm::front::state<>{};
        struct state_stopped : public boost::msm::front::state<>{};

        void action_init(const event_init& init);
        void action_play(const event_play& play);
        void action_pause(const event_pause& pause);
        void action_stop(const event_stop& stop);
        void action_virtualise(const event_virtualise& virtualise);
        void action_devirtualuse(const event_devirtualise& devirtualise);

        typedef state_uninit initial_state; // the initial state of the player SM

        // Start, Event, Next, Action, Guard
        struct transition_table : boost::mpl::vector
            <
                a_row<state_uninit, event_init, state_init, &node_instance::action_init>,
                a_row<state_init, event_play, state_playing, &node_instance::action_play>,
                a_row<state_playing, event_stop, state_stopped, &node_instance::action_stop>
            >
        {};

        bool init(const init_node_instance& initData);
        bool play();

        void update();

        auto is_playing() const -> bool;
        auto is_stopped() const -> bool;
        auto get_state() const -> node_instance_state;

        std::shared_ptr<node> getReferencingNode() const noexcept { return m_referencingNode; }
        node_instance* get_parent() const noexcept { return m_parent.parent.get(); }
        sc_node_group* getBus() const noexcept { return m_nodeGroup.nodeGroup.get(); }

    private:
        void setVolume(float oldVolume, float newVolume);
        void setPitch(float oldPitch, float newPitch);
        void setLowpass(float oldLowpass, float newLowpass);
        void setHighpass(float oldHighpass, float newHighpass);

        std::shared_ptr<node> m_referencingNode = nullptr;
        voice* m_owningVoice                    = nullptr;
        node_instance_state m_state               = node_instance_state::UNINIT;
        node_group_instance m_nodeGroup;
        parent_node_owner m_parent;
        children_node_owner m_children;
        std::unique_ptr<sc_sound_instance, SC_SOUND_INSTANCE_DELETER> m_soundInstance;
        unsigned int m_numTimesPlayed = 0;

        REGISTER_REFLECTION(node_instance, sbk::core::object)
    };
}  // namespace sbk::engine
