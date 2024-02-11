#include "sound_bakery/gameobject/gameobject.h"
#include "sound_bakery/node/bus/bus.h"
#include "sound_bakery/node/container/container.h"
#include "sound_bakery/voice/node_instance.h"
#include "sound_bakery/voice/voice.h"

using namespace SB::Engine;

static void addDspToNodeGroup(SC_NODE_GROUP* nodeGroup, SC_DSP** dsp, const SC_DSP_CONFIG& config)
{
    assert(dsp != nullptr);
    SC_System_CreateDSP(SB::Engine::System::getChef(), &config, dsp);
    SC_NodeGroup_AddDSP(nodeGroup, *dsp, SC_DSP_INDEX_HEAD);
}

bool NodeGroupInstance::initNodeGroup(const NodeBase& node)
{
    if (nodeGroup != nullptr)
    {
        return true;
    }

    SC_NODE_GROUP* nGroup                 = nullptr;
    const SC_RESULT createNodeGroupResult = SC_System_CreateNodeGroup(SB::Engine::System::getChef(), &nGroup);

    if (createNodeGroupResult != MA_SUCCESS)
    {
        return false;
    }

    nodeGroup.reset(nGroup);

    addDspToNodeGroup(nodeGroup.get(), &lowpass, SC_DSP_Config_Init(SC_DSP_TYPE_LOWPASS));
    addDspToNodeGroup(nodeGroup.get(), &highpass, SC_DSP_Config_Init(SC_DSP_TYPE_HIGHPASS));

    for (const SB::Core::DatabasePtr<SB::Engine::EffectDescription>& desc :
         node.tryConvertObject<Node>()->m_effectDescriptions)
    {
        if (desc.lookup() == false)
        {
            continue;
        }

        SC_DSP* dsp = nullptr;

        addDspToNodeGroup(nodeGroup.get(), &dsp, *desc->getConfig());

        int index = 0;
        for (const SB::Engine::EffectParameterDescription& parameter : desc->getParameters())
        {
            switch (parameter.m_parameter.m_type)
            {
                case SC_DSP_PARAMETER_TYPE_FLOAT:
                    SC_DSP_SetParameterFloat(dsp, index++, parameter.m_parameter.m_float.m_value);
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
                parent        = masterBus->lockAndCopy();
                createdParent = parent->init(masterBus->tryConvertObject<NodeBase>(), NodeInstanceType::BUS, owningVoice);
            }
            break;
        }
        case SB_NODE_MIDDLE:
        {
            if (SB::Engine::NodeBase* parentNode = thisNode.parent())
            {
                parent        = std::make_shared<NodeInstance>();
                createdParent = parent->init(parentNode->tryConvertObject<NodeBase>(), NodeInstanceType::BUS, owningVoice);
            }
            break;
        }
        case SB_NODE_TOP:
        {
            if (SB::Engine::NodeBase* output = thisNode.outputBus())
            {
                if (SB::Engine::Bus* bus = output->tryConvertObject<Bus>())
                {
                    parent        = bus->lockAndCopy();
                    createdParent = parent->init(bus->tryConvertObject<NodeBase>(), NodeInstanceType::BUS, owningVoice);
                }
            }
            break;
        }
    }

    return createdParent && parent != nullptr;
}

////////////////////////////////////////////////////////////////////////////

bool ChildrenNodeOwner::createChildren(const NodeBase& thisNode, Voice* owningVoice)
{
    bool success = false;

    if (const Container* container = thisNode.tryConvertObject<Container>(); container != nullptr && owningVoice != nullptr)
    {
        GatherChildrenContext context;
        context.parameters = owningVoice->getOwningGameObject()->getLocalParameters();

        container->gatherChildrenForPlay(context);

        childrenNodes.reserve(context.sounds.size());

        for (const auto* const child : context.sounds)
        {
            if (child == nullptr)
            {
                continue;
            }

            childrenNodes.push_back(std::make_shared<NodeInstance>());
            childrenNodes.back()->init(SB::Core::DatabasePtr<NodeBase>(child->getDatabaseID()), NodeInstanceType::CHILD, owningVoice);
        }

        success = true;
    }

    return success;
}