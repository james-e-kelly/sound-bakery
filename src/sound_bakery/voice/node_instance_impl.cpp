#include "sound_bakery/gameobject/gameobject.h"
#include "sound_bakery/node/bus/bus.h"
#include "sound_bakery/node/container/container.h"
#include "sound_bakery/voice/node_instance.h"
#include "sound_bakery/voice/voice.h"

using namespace sbk::engine;

static void addDspToNodeGroup(sc_node_group* nodeGroup, sc_dsp** dsp, const sc_dsp_config& config)
{
    assert(dsp != nullptr);
    sc_system_create_dsp(sbk::engine::system::get(), &config, dsp);
    sc_node_group_add_dsp(nodeGroup, *dsp, SC_DSP_INDEX_HEAD);
}

bool NodeGroupInstance::initNodeGroup(const NodeBase& node)
{
    if (nodeGroup != nullptr)
    {
        return true;
    }

    sc_node_group* nGroup                 = nullptr;
    const sc_result createNodeGroupResult = sc_system_create_node_group(sbk::engine::system::get(), &nGroup);

    if (createNodeGroupResult != MA_SUCCESS)
    {
        return false;
    }

    nodeGroup.reset(nGroup);

    addDspToNodeGroup(nodeGroup.get(), &lowpass, sc_dsp_config_init(SC_DSP_TYPE_LOWPASS));
    addDspToNodeGroup(nodeGroup.get(), &highpass, sc_dsp_config_init(SC_DSP_TYPE_HIGHPASS));

    for (const sbk::core::DatabasePtr<sbk::engine::effect_description>& desc :
         node.try_convert_object<Node>()->m_effectDescriptions)
    {
        if (desc.lookup() == false)
        {
            continue;
        }

        sc_dsp* dsp = nullptr;

        addDspToNodeGroup(nodeGroup.get(), &dsp, *desc->get_config());

        int index = 0;
        for (const sbk::engine::effect_parameter_description& parameter : desc->get_parameters())
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
            // @TODO Fix up master bus
            // const sbk::core::DatabasePtr<Bus>& masterBus = sbk::engine::system::get()->getMasterBus();

            // if (masterBus.lookup() && masterBus.id() != thisNode.get_database_id())
            //{
            //     parent = masterBus->lockAndCopy();

            //    InitNodeInstance initData;
            //    initData.refNode     = masterBus->try_convert_object<NodeBase>();
            //    initData.type        = NodeInstanceType::BUS;
            //    initData.owningVoice = owningVoice;

            //    createdParent = parent->init(initData);
            //}
            break;
        }
        case SB_NODE_MIDDLE:
        {
            if (sbk::engine::NodeBase* parentNode = thisNode.parent())
            {
                parent = std::make_shared<NodeInstance>();

                InitNodeInstance initData;
                initData.refNode     = parentNode->try_convert_object<NodeBase>();
                initData.type        = NodeInstanceType::BUS;
                initData.owningVoice = owningVoice;

                createdParent = parent->init(initData);
            }
            break;
        }
        case SB_NODE_TOP:
        {
            if (sbk::engine::NodeBase* output = thisNode.outputBus())
            {
                if (sbk::engine::Bus* bus = output->try_convert_object<Bus>())
                {
                    parent = bus->lockAndCopy();

                    InitNodeInstance initData;
                    initData.refNode     = bus->try_convert_object<NodeBase>();
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

    if (const Container* container = thisNode.try_convert_object<Container>();
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
            initData.refNode           = sbk::core::DatabasePtr<NodeBase>(child->get_database_id());
            initData.type              = NodeInstanceType::CHILD;
            initData.owningVoice       = owningVoice;
            initData.parentForChildren = thisNodeInstance;

            childrenNodes.back()->init(initData);
        }

        success = true;
    }

    return success;
}