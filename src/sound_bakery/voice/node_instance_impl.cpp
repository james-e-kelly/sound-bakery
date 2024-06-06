#include "sound_bakery/gameobject/gameobject.h"
#include "sound_bakery/node/bus/bus.h"
#include "sound_bakery/node/container/container.h"
#include "sound_bakery/voice/node_instance.h"
#include "sound_bakery/voice/voice.h"

using namespace SB::Engine;

static void addDspToNodeGroup(sc_node_group* nodeGroup, sc_dsp** dsp, const sc_dsp_config& config)
{
    assert(dsp != nullptr);
    sc_system_create_dsp(SB::Engine::System::getChef(), &config, dsp);
    sc_node_group_add_dsp(nodeGroup, *dsp, SC_DSP_INDEX_HEAD);
}

bool NodeGroupInstance::initNodeGroup(const NodeBase& node)
{
    if (nodeGroup != nullptr)
    {
        return true;
    }

    sc_node_group* nGroup                 = nullptr;
    const sc_result createNodeGroupResult = sc_system_create_node_group(SB::Engine::System::getChef(), &nGroup);

    if (createNodeGroupResult != MA_SUCCESS)
    {
        return false;
    }

    nodeGroup.reset(nGroup);

    addDspToNodeGroup(nodeGroup.get(), &lowpass, sc_dsp_config_init(SC_DSP_TYPE_LOWPASS));
    addDspToNodeGroup(nodeGroup.get(), &highpass, sc_dsp_config_init(SC_DSP_TYPE_HIGHPASS));

    for (const SB::Core::DatabasePtr<SB::Engine::EffectDescription>& desc :
         node.tryConvertObject<Node>()->m_effectDescriptions)
    {
        if (desc.lookup() == false)
        {
            continue;
        }

        sc_dsp* dsp = nullptr;

        addDspToNodeGroup(nodeGroup.get(), &dsp, *desc->getConfig());

        int index = 0;
        for (const SB::Engine::EffectParameterDescription& parameter : desc->getParameters())
        {
            switch (parameter.m_parameter.type)
            {
                case SC_DSP_PARAMETER_TYPE_FLOAT:
                    sc_dsp_set_parameter_float(dsp, index++, parameter.m_parameter.floatParameter.value);
                    break;
            }
        }
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////

bool ParentNodeOwner::createParent(const NodeBase& thisNode, Voice* owningVoice)
{
    bool createdParent = false;

    switch (thisNode.getNodeStatus())
    {
        case SB_NODE_NULL:
        {
            const SB::Core::DatabasePtr<Bus>& masterBus = SB::Engine::System::get()->getMasterBus();

            if (masterBus.lookup() && masterBus.id() != thisNode.getDatabaseID())
            {
                parent = masterBus->lockAndCopy();

                InitNodeInstance initData;
                initData.refNode     = masterBus->tryConvertObject<NodeBase>();
                initData.type        = NodeInstanceType::BUS;
                initData.owningVoice = owningVoice;

                createdParent = parent->init(initData);
            }
            break;
        }
        case SB_NODE_MIDDLE:
        {
            if (SB::Engine::NodeBase* parentNode = thisNode.parent())
            {
                parent = std::make_shared<NodeInstance>();

                InitNodeInstance initData;
                initData.refNode     = parentNode->tryConvertObject<NodeBase>();
                initData.type        = NodeInstanceType::BUS;
                initData.owningVoice = owningVoice;

                createdParent = parent->init(initData);
            }
            break;
        }
        case SB_NODE_TOP:
        {
            if (SB::Engine::NodeBase* output = thisNode.outputBus())
            {
                if (SB::Engine::Bus* bus = output->tryConvertObject<Bus>())
                {
                    parent = bus->lockAndCopy();

                    InitNodeInstance initData;
                    initData.refNode     = bus->tryConvertObject<NodeBase>();
                    initData.type        = NodeInstanceType::BUS;
                    initData.owningVoice = owningVoice;

                    createdParent = parent->init(initData);
                }
            }
            break;
        }
    }

    return createdParent && parent != nullptr;
}

////////////////////////////////////////////////////////////////////////////

bool ChildrenNodeOwner::createChildren(const NodeBase& thisNode,
                                       Voice* owningVoice,
                                       NodeInstance* thisNodeInstance,
                                       unsigned int numTimesPlayed)
{
    bool success = false;

    if (const Container* container = thisNode.tryConvertObject<Container>();
        container != nullptr && owningVoice != nullptr)
    {
        GatherChildrenContext context;
        context.numTimesPlayed = numTimesPlayed;
        context.parameters     = owningVoice->getOwningGameObject()->getLocalParameters();

        container->gatherChildrenForPlay(context);

        childrenNodes.reserve(context.sounds.size());

        for (const auto* const child : context.sounds)
        {
            if (child == nullptr)
            {
                continue;
            }

            childrenNodes.push_back(std::make_shared<NodeInstance>());

            InitNodeInstance initData;
            initData.refNode           = SB::Core::DatabasePtr<NodeBase>(child->getDatabaseID());
            initData.type              = NodeInstanceType::CHILD;
            initData.owningVoice       = owningVoice;
            initData.parentForChildren = thisNodeInstance;

            childrenNodes.back()->init(initData);
        }

        success = true;
    }

    return success;
}