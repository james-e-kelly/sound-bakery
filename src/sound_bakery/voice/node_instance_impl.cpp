#include "sound_bakery/gameobject/gameobject.h"
#include "sound_bakery/node/bus/bus.h"
#include "sound_bakery/node/container/container.h"
#include "sound_bakery/system.h"
#include "sound_bakery/voice/node_instance.h"
#include "sound_bakery/voice/voice.h"

using namespace sbk::engine;

static void addDspToNodeGroup(sc_node_group* nodeGroup, sc_dsp** dsp, const sc_dsp_config& config)
{
    BOOST_ASSERT(dsp != nullptr);
    sc_system_create_dsp(sbk::engine::system::get(), &config, dsp);
    sc_node_group_add_dsp(nodeGroup, *dsp, SC_DSP_INDEX_HEAD);
}

bool node_group_instance::init_node_group(const node_base& node)
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

    for (const sbk::core::database_ptr<sbk::engine::effect_description>& desc :
         node.try_convert_object<sbk::engine::node>()->m_effectDescriptions)
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

bool parent_node_owner::create_parent(const node_base& thisNode, voice* owningVoice)
{
    bool createdParent = false;

    switch (thisNode.getNodeStatus())
    {
        case SB_NODE_NULL:
        {
            const sbk::core::database_ptr<bus>& masterBus = sbk::engine::system::get()->get_master_bus();

            if (masterBus.lookup())
            {
                if (masterBus.id() != thisNode.get_database_id())
                {
                    parent = masterBus->lockAndCopy();

                    init_node_instance initData;
                    initData.refNode     = masterBus->try_convert_object<node_base>();
                    initData.type        = NodeInstanceType::BUS;
                    initData.owningVoice = owningVoice;

                    createdParent = parent->init(initData);
                }
            }
            else
            {
                SPDLOG_ERROR("Master Bus was invalid when creating node graph");
            }
            break;
        }
        case SB_NODE_MIDDLE:
        {
            if (sbk::engine::node_base* parentNode = thisNode.get_parent())
            {
                parent = std::make_shared<node_instance>();

                init_node_instance initData;
                initData.refNode     = parentNode->try_convert_object<node_base>();
                initData.type        = NodeInstanceType::BUS;
                initData.owningVoice = owningVoice;

                createdParent = parent->init(initData);
            }
            break;
        }
        case SB_NODE_TOP:
        {
            if (sbk::engine::node_base* output = thisNode.get_output_bus())
            {
                if (sbk::engine::bus* bus = output->try_convert_object<sbk::engine::bus>())
                {
                    parent = bus->lockAndCopy();

                    init_node_instance initData;
                    initData.refNode     = bus->try_convert_object<node_base>();
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

bool children_node_owner::createChildren(const node_base& thisNode,
                                       voice* owningVoice,
                                       node_instance* thisNodeInstance,
                                       unsigned int numTimesPlayed)
{
    bool success = false;

    if (const container* const container = thisNode.try_convert_object<sbk::engine::container>();
        container != nullptr && owningVoice != nullptr)
    {
        gather_children_context context;
        context.numTimesPlayed = numTimesPlayed;
        context.parameters     = owningVoice->get_owning_game_object()->get_local_parameters();

        container->gather_children_for_play(context);

        childrenNodes.reserve(context.sounds.size());

        for (const auto* const child : context.sounds)
        {
            if (child == nullptr)
            {
                continue;
            }

            childrenNodes.push_back(std::make_shared<node_instance>());

            init_node_instance initData;
            initData.refNode           = sbk::core::database_ptr<node_base>(child->get_database_id());
            initData.type              = NodeInstanceType::CHILD;
            initData.owningVoice       = owningVoice;
            initData.parentForChildren = thisNodeInstance;

            childrenNodes.back()->init(initData);
        }

        success = true;
    }

    return success;
}