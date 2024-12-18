#pragma once

#include "sound_bakery/core/core_include.h"
#include "sound_bakery/system.h"

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
    struct SB_CLASS NodeGroupInstance
    {
        bool initNodeGroup(const node_base& node);

        sc_dsp* lowpass  = nullptr;
        sc_dsp* highpass = nullptr;
        std::unique_ptr<sc_node_group, SC_NODE_GROUP_DELETER> nodeGroup;
    };

    /**
     * @brief Owns a get_parent node instance.
     */
    struct SB_CLASS ParentNodeOwner
    {
        bool createParent(const node_base& thisNode, voice* owningVoice);

        std::shared_ptr<node_instance> parent;
    };

    /**
     * @brief Owns a list of child node instances.
     */
    struct SB_CLASS ChildrenNodeOwner
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

    enum class NodeInstanceState
    {
        UNINIT,

        STOPPED,
        STOPPING,
        PAUSED,
        VIRTUAL,

        PLAYING
    };

    struct SB_CLASS InitNodeInstance
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

    /**
     * @brief NodeInstances represent runtime versions of Nodes, either
     * containers or busses
     */
    class SB_CLASS node_instance : public sbk::core::object
    {
    public:
        ~node_instance();

        bool init(const InitNodeInstance& initData);
        bool play();

        void update();

        bool isPlaying() const
        {
            return m_state == NodeInstanceState::PLAYING || m_state == NodeInstanceState::STOPPING;
        }

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
        NodeInstanceState m_state               = NodeInstanceState::UNINIT;
        NodeGroupInstance m_nodeGroup;
        ParentNodeOwner m_parent;
        ChildrenNodeOwner m_children;
        std::unique_ptr<sc_sound_instance, SC_SOUND_INSTANCE_DELETER> m_soundInstance;
        unsigned int m_numTimesPlayed = 0;

        REGISTER_REFLECTION(node_instance, sbk::core::object)
    };
}  // namespace sbk::engine
