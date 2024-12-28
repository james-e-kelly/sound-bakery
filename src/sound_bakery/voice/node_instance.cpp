#include "node_instance.h"

#include "sound_bakery/gameobject/gameobject.h"
#include "sound_bakery/maths/easing.h"
#include "sound_bakery/node/bus/bus.h"
#include "sound_bakery/node/container/sequence_container.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/sound/sound.h"
#include "sound_bakery/system.h"
#include "sound_bakery/voice/voice.h"

DEFINE_REFLECTION(sbk::engine::node_instance)

sbk::engine::node_instance_fsm::~node_instance_fsm()
{
    if (m_referencingNode != nullptr)
    {
        m_referencingNode->m_volume.get_delegate().RemoveObject(this);
        m_referencingNode->m_pitch.get_delegate().RemoveObject(this);
        m_referencingNode->m_lowpass.get_delegate().RemoveObject(this);
        m_referencingNode->m_highpass.get_delegate().RemoveObject(this);
    }
}

// ACTIONS //

auto sbk::engine::node_instance_fsm::action_init(const event_init& init) -> void
{
    m_referencingNode = std::static_pointer_cast<sbk::engine::node>(init.refNode.shared());
    init_node_group(init);
    init_callbacks();

    switch (init.type)
    {
        case node_instance_type::child:
        {
            init_child();
            break;
        }
        case node_instance_type::bus:
        {
            init_parent();
            break;
        }
        case node_instance_type::main:
        {
            init_parent();
            init_child();
            break;
        }
    }

    if (m_parent)
    {
        sc_node_group_set_parent(m_nodeGroup.nodeGroup.get(), m_parent->get_bus());
    }
    else if (init.parentForChildren)
    {
        sc_node_group_set_parent(m_nodeGroup.nodeGroup.get(), init.parentForChildren->get_bus());
    }
}

auto sbk::engine::node_instance_fsm::action_play(const event_play& play) -> void
{
    if (m_referencingNode->get_object_type() == rttr::type::get<sound_container>())
    {
        sbk::engine::sound_container* const soundContainer  = m_referencingNode->try_convert_object<sound_container>();
        sbk::engine::sound* const engineSound               = soundContainer->get_sound();
        sc_sound* const sound                               = engineSound != nullptr ? engineSound->get_sound() : nullptr;

        const sc_result playSoundResult = sc_system_play_sound(sbk::engine::system::get(), sound, ztd::out_ptr::out_ptr(m_soundInstance),
                                                         m_nodeGroup.nodeGroup.get(), MA_FALSE);

        BOOST_ASSERT(playSoundResult == MA_SUCCESS);
    }
    else
    {
        std::for_each(m_children.begin(), m_children.end(), [](const auto& child) { child->play(); });
    }
}

auto sbk::engine::node_instance_fsm::action_stop(const event_stop& stop) -> void
{ 
    m_soundInstance.release(); 
    m_children.clear();
    m_parent.reset();
}

// GUARDS //

auto sbk::engine::node_instance_fsm::guard_init(const event_init& init) -> bool
{
    if (!init.refNode.lookup())
    {
        return false;
    }

    if (!init.refNode->get_object_type().is_derived_from<sbk::engine::node>())
    {
        return false;
    }

    return true;
}

// API //

auto sbk::engine::node_instance::init(const event_init& init) -> sb_result 
{ 
    m_stateMachine.m_owner = this;
    m_stateMachine.start();
    return m_stateMachine.process_event(init) ? MA_SUCCESS : MA_ERROR; 
}

auto sbk::engine::node_instance::play() -> sb_result
{
    m_stateMachine.process_event(event_play());
    return MA_SUCCESS;
}

auto sbk::engine::node_instance::update() -> sb_result
{
    m_stateMachine.process_event(event_update());
    return MA_SUCCESS;
}

auto sbk::engine::node_instance::stop(float fadeTime) -> sb_result
{
    m_stateMachine.process_event(event_stop{.stopTime = fadeTime});
    return MA_SUCCESS;
}

// QUERIES

auto sbk::engine::node_instance::is_playing() const -> bool
{
    return m_stateMachine.is_flag_active<flag_playing>();
}

auto sbk::engine::node_instance::is_stopped() const -> bool
{ 
    return m_stateMachine.is_flag_active<flag_stopped>(); 
}

auto sbk::engine::node_instance::get_referencing_node() const noexcept -> std::shared_ptr<node>
{
    return m_stateMachine.m_referencingNode;
}

auto sbk::engine::node_instance::get_parent() const noexcept -> node_instance* { return m_stateMachine.m_parent.get(); }

auto sbk::engine::node_instance::get_bus() const noexcept -> sc_node_group*
{
    return m_stateMachine.m_nodeGroup.nodeGroup.get();
}

// INIT //

auto sbk::engine::node_instance_fsm::add_dsp_to_node_group(sc_node_group* nodeGroup,
                                                       sc_dsp** dsp,
                                                       const sc_dsp_config& config) -> sb_result
{
    SC_CHECK_ARG(nodeGroup != nullptr);
    SC_CHECK_ARG(dsp != nullptr);
    SC_CHECK_ARG(config.vtable != nullptr);
    SC_CHECK_RESULT(sc_system_create_dsp(sbk::engine::system::get(), &config, dsp));
    SC_CHECK_RESULT(sc_node_group_add_dsp(nodeGroup, *dsp, SC_DSP_INDEX_HEAD));
    return MA_SUCCESS;
}

auto sbk::engine::node_instance_fsm::init_node_group(const event_init& init) -> sb_result
{
    SC_CHECK_RESULT(sc_system_create_node_group(sbk::engine::system::get(), ztd::out_ptr::out_ptr(m_nodeGroup.nodeGroup)));
    SC_CHECK_RESULT(add_dsp_to_node_group(m_nodeGroup.nodeGroup.get(), &m_nodeGroup.lowpass, sc_dsp_config_init(SC_DSP_TYPE_LOWPASS)));
    SC_CHECK_RESULT(add_dsp_to_node_group(m_nodeGroup.nodeGroup.get(), &m_nodeGroup.highpass, sc_dsp_config_init(SC_DSP_TYPE_HIGHPASS)));

    for (const sbk::core::database_ptr<sbk::engine::effect_description>& desc : m_referencingNode->m_effectDescriptions)
    {
        if (desc.lookup())
        {
            sc_dsp* dsp = nullptr;
            add_dsp_to_node_group(m_nodeGroup.nodeGroup.get(), &dsp, *desc->get_config());

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
    }

    return MA_SUCCESS;
}

void sbk::engine::node_instance_fsm::init_parent()
{
    sbk::engine::node_base* nodeToReference = nullptr;

    switch (m_referencingNode->getNodeStatus())
    {
        case SB_NODE_TOP:
            nodeToReference = m_referencingNode->get_output_bus();
            BOOST_ASSERT_MSG(nodeToReference, "Output bus must be valid");
            break;
        case SB_NODE_MIDDLE:
            nodeToReference = m_referencingNode->get_parent();
            BOOST_ASSERT_MSG(nodeToReference, "Parent must be valid");
            break;
        case SB_NODE_NULL:
            nodeToReference = sbk::engine::system::get()->get_master_bus();
            BOOST_ASSERT_MSG(nodeToReference, "Master Bus invalid");
            break;
    }

    event_init initData{.refNode = nodeToReference, .type = node_instance_type::bus};
    m_parent = m_owner->create_runtime_object<sbk::engine::node_instance>();
    m_parent->init(initData);
}

void sbk::engine::node_instance_fsm::init_child()
{
    if (const container* const container = m_referencingNode->try_convert_object<sbk::engine::container>())
    {
        gather_children_context context;
        context.numTimesPlayed = m_numTimesPlayed;
        context.parameters = m_owner->try_convert_object<sbk::engine::voice>()
                                 ->get_owning_game_object()
                                 ->get_local_parameters();

        container->gather_children_for_play(context);

        m_children.reserve(context.sounds.size());

        for (sbk::engine::container* const child : context.sounds)
        {
            if (child)
            {
                m_children.push_back(m_owner->create_runtime_object<sbk::engine::node_instance>());

                event_init childInit;
                childInit.parentForChildren = m_owner;
                childInit.type              = node_instance_type::child;
                childInit.refNode           = child->get_database_id();

                m_children.back()->init(childInit);
            }
        }
    }
}

void sbk::engine::node_instance_fsm::init_callbacks()
{
    m_referencingNode->m_volume.get_delegate().AddRaw(this, &node_instance_fsm::set_volume);
    m_referencingNode->m_pitch.get_delegate().AddRaw(this, &node_instance_fsm::set_pitch);
    m_referencingNode->m_lowpass.get_delegate().AddRaw(this, &node_instance_fsm::set_lowpass);
    m_referencingNode->m_highpass.get_delegate().AddRaw(this, &node_instance_fsm::set_highpass);

    set_volume(0.0F, m_referencingNode->m_volume.get());
    set_pitch(0.0F, m_referencingNode->m_pitch.get());
    set_lowpass(0.0F, m_referencingNode->m_lowpass.get());
    set_highpass(0.0F, m_referencingNode->m_highpass.get());
}

// CALLBACKS //

void sbk::engine::node_instance_fsm::set_volume(float oldVolume, float newVolume)
{
    (void)oldVolume;

    if (m_nodeGroup.nodeGroup)
    {
        ma_sound_group_set_volume((ma_sound_group*)m_nodeGroup.nodeGroup->fader->state->userData, newVolume);
    }
}

void sbk::engine::node_instance_fsm::set_pitch(float oldPitch, float newPitch)
{
    (void)oldPitch;

    if (m_nodeGroup.nodeGroup)
    {
        ma_sound_group_set_pitch((ma_sound_group*)m_nodeGroup.nodeGroup->fader->state->userData, newPitch);
    }
}

void sbk::engine::node_instance_fsm::set_lowpass(float oldLowpass, float newLowpass)
{
    (void)oldLowpass;

    const double percentage    = sbk::maths::easeOutCubic(newLowpass / 100.0);
    const double lowpassCutoff = (19980 - (19980.0 * percentage)) + 20.0;
    BOOST_ASSERT(lowpassCutoff >= 20.0);

    sc_dsp_set_parameter_float(m_nodeGroup.lowpass, SC_DSP_LOWPASS_CUTOFF, static_cast<float>(lowpassCutoff));
}

void sbk::engine::node_instance_fsm::set_highpass(float oldHighpass, float newHighpass)
{
    (void)oldHighpass;

    const double percentage     = sbk::maths::easeInCubic(newHighpass / 100.0);
    const double highpassCutoff = (19980.0 * percentage) + 20.0;
    BOOST_ASSERT(highpassCutoff >= 20.0);

    sc_dsp_set_parameter_float(m_nodeGroup.highpass, SC_DSP_HIGHPASS_CUTOFF, static_cast<float>(highpassCutoff));
}
