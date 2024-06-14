#pragma once

#include "sound_bakery/core/core_include.h"
#include "sound_bakery/system.h"

namespace SB::Engine
{
    class Bus;
    class Container;
    class Node;
    class NodeBase;
    class NodeInstance;
    class Sound;
    class SoundContainer;
    class Voice;

    /**
     * @brief Owns a node group and applies DSP effects to it.
     */
    struct SB_CLASS NodeGroupInstance
    {
        bool initNodeGroup(const NodeBase& node);

        sc_dsp* lowpass  = nullptr;
        sc_dsp* highpass = nullptr;
        std::unique_ptr<sc_node_group, SC_NODE_GROUP_DELETER> nodeGroup;
    };

    /**
     * @brief Owns a parent node instance.
     */
    struct SB_CLASS ParentNodeOwner
    {
        bool createParent(const NodeBase& thisNode, Voice* owningVoice);

        std::shared_ptr<NodeInstance> parent;
    };

    /**
     * @brief Owns a list of child node instances.
     */
    struct SB_CLASS ChildrenNodeOwner
    {
        bool createChildren(const NodeBase& thisNode,
                            Voice* owningVoice,
                            NodeInstance* thisNodeInstance,
                            unsigned int numTimesPlayed);

        std::vector<std::shared_ptr<NodeInstance>> childrenNodes;
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
        SB::Core::DatabasePtr<NodeBase> refNode;

        /**
         * @brief Type of node to create.
         *
         * Different types of nodes initialize differently. For example, parent nodes only create more parents.
         * Children create more children.
         */
        NodeInstanceType type = NodeInstanceType::MAIN;

        /**
         * @brief Voice owner.
         */
        Voice* owningVoice = nullptr;

        /**
         * @brief Parent node instance for this node instance.
         *
         * Used when initializing children so it can join the DSP graph correctly.
         */
        NodeInstance* parentForChildren = nullptr;
    };

    /**
     * @brief NodeInstances represent runtime versions of Nodes, either
     * containers or busses
     */
    class SB_CLASS NodeInstance : public SB::Core::object
    {
    public:
        ~NodeInstance();

        bool init(const InitNodeInstance& initData);
        bool play();

        void update();

        bool isPlaying() const
        {
            return m_state == NodeInstanceState::PLAYING || m_state == NodeInstanceState::STOPPING;
        }

        std::shared_ptr<Node> getReferencingNode() const noexcept { return m_referencingNode; }
        NodeInstance* getParent() const noexcept { return m_parent.parent.get(); }
        sc_node_group* getBus() const noexcept { return m_nodeGroup.nodeGroup.get(); }

    private:
        void setVolume(float oldVolume, float newVolume);
        void setPitch(float oldPitch, float newPitch);
        void setLowpass(float oldLowpass, float newLowpass);
        void setHighpass(float oldHighpass, float newHighpass);

        std::shared_ptr<Node> m_referencingNode = nullptr;
        Voice* m_owningVoice                    = nullptr;
        NodeInstanceState m_state               = NodeInstanceState::UNINIT;
        NodeGroupInstance m_nodeGroup;
        ParentNodeOwner m_parent;
        ChildrenNodeOwner m_children;
        std::unique_ptr<sc_sound_instance, SC_SOUND_INSTANCE_DELETER> m_soundInstance;
        unsigned int m_numTimesPlayed = 0;

        REGISTER_REFLECTION(NodeInstance, SB::Core::object)
    };
}  // namespace SB::Engine
