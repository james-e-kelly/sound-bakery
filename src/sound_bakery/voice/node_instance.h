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
    struct NodeGroupInstance
    {
        bool initNodeGroup(const NodeBase& node);

        SC_DSP* lowpass  = nullptr;
        SC_DSP* highpass = nullptr;
        std::unique_ptr<SC_NODE_GROUP, SC_NODE_GROUP_DELETER> nodeGroup;
    };

    /**
     * @brief Owns a parent node instance.
     */
    struct ParentNodeOwner
    {
        bool createParent(const NodeBase& thisNode, Voice* owningVoice);

        std::shared_ptr<NodeInstance> parent;
    };

    /**
     * @brief Owns a list of child node instances.
     */
    struct ChildrenNodeOwner
    {
        bool createChildren(const NodeBase& thisNode, Voice* owningVoice);

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

    /**
     * @brief NodeInstances represent runtime versions of Nodes, either
     * containers or busses
     */
    class NodeInstance : public SB::Core::Object
    {
    public:
        ~NodeInstance();

        bool init(const SB::Core::DatabasePtr<NodeBase>& refNode, NodeInstanceType type, Voice* owningVoice);
        bool play();

        void update();

        bool isPlaying() const
        {
            return m_state == NodeInstanceState::PLAYING || m_state == NodeInstanceState::STOPPING;
        }

        SB::Core::DatabasePtr<Node> getReferencingNode() const noexcept { return m_referencingNode; }
        NodeInstance* getParent() const noexcept { return m_parent.parent.get(); }
        SC_NODE_GROUP* getBus() const noexcept { return m_nodeGroup.nodeGroup.get(); }

    private:
        void setVolume(float oldVolume, float newVolume);
        void setPitch(float oldPitch, float newPitch);
        void setLowpass(float oldLowpass, float newLowpass);
        void setHighpass(float oldHighpass, float newHighpass);

        Node* m_referencingNode   = nullptr;
        Voice* m_owningVoice      = nullptr;
        NodeInstanceState m_state = NodeInstanceState::UNINIT;
        NodeGroupInstance m_nodeGroup;
        ParentNodeOwner m_parent;
        ChildrenNodeOwner m_children;
        std::unique_ptr<SC_SOUND_INSTANCE, SC_SOUND_INSTANCE_DELETER> m_soundInstance;

        RTTR_ENABLE(SB::Core::Object)
    };
}  // namespace SB::Engine
