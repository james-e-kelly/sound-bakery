#include "node_instance.h"

#include "sound_bakery/error/result.h"
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
    ZoneScoped;
    m_referencingNode = std::static_pointer_cast<sbk::engine::node>(init.refNode.shared());
    (void)init_node_group(init);
    init_callbacks();

    switch (init.type)
    {
        case node_instance_type::child:
        {
            (void)init_child();
            break;
        }
        case node_instance_type::bus:
        {
            (void)init_parent();
            break;
        }
        case node_instance_type::main:
        {
            (void)init_parent();
            (void)init_child();
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
    ZoneScoped;
    if (m_referencingNode->get_object_type() == rttr::type::get<sound_container>())
    {
        const sbk::engine::sound_container* const soundContainer = m_referencingNode->try_convert_object<sound_container>();
        auto engineSound                                         = soundContainer->get_sound();
        sc_sound* const sound                                    = engineSound ? engineSound->get_sound() : nullptr;

        if (sound)
        {
            sc_sound_instance* soundInstance = nullptr;
            if (const sbk_status playSoundResult = sc_system_play_sound(sbk::engine::system::get(), sound, &soundInstance, m_nodeGroup.nodeGroup.get(), MA_FALSE); playSoundResult != SBK_SUCCESS)
            {
                sbk::log_error(playSoundResult, "sc_system_play_sound");
            }
            m_soundInstance.reset(soundInstance);
        }
        else
        {
            SBK_WARN("No sound to play");
        }
    }
    else
    {
        std::for_each(m_children.begin(), m_children.end(), [](const auto& child)
                      { (void)child->play(); });
    }
}

auto sbk::engine::node_instance_fsm::action_stop(const event_stop& stop) -> void
{
    ZoneScoped;
    m_soundInstance.reset();
    m_children.clear();
    m_parent.reset();
}

// GUARDS //

auto sbk::engine::node_instance_fsm::guard_init(const event_init& init) -> bool
{
    ZoneScoped;
    if (const auto refNode = init.refNode.shared())
    {
        return refNode->get_object_type().is_derived_from<sbk::engine::node>();
    }

    return false;
}

// API //

auto sbk::engine::node_instance::init(const event_init& init) -> sbk::result<void>
{
    ZoneScoped;
    m_stateMachine.m_gameObject = init.m_owningGameObject;
    m_stateMachine.m_owner      = this;
    m_stateMachine.start();
    SBK_CHECK(m_stateMachine.process_event(init), SBK_ERR_BAKERY);
    return sbk::ok();
}

auto sbk::engine::node_instance::play() -> sbk::result<void>
{
    ZoneScoped;
    m_stateMachine.process_event(event_play());
    return sbk::ok();
}

auto sbk::engine::node_instance::update() -> sbk::result<void>
{
    ZoneScoped;
    m_stateMachine.process_event(event_update());
    return sbk::ok();
}

auto sbk::engine::node_instance::stop(float fadeTime) -> sbk::result<void>
{
    ZoneScoped;
    m_stateMachine.process_event(event_stop{.stopTime = fadeTime});
    return sbk::ok();
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
                                                           const sc_dsp_config& config) -> sbk::result<void>
{
    ZoneScoped;
    SBK_CHECK(nodeGroup != nullptr, SBK_ERR_INVALID_PARAMETER);
    SBK_CHECK(dsp != nullptr, SBK_ERR_INVALID_PARAMETER);
    SBK_CHECK(config.vtable != nullptr, SBK_ERR_INVALID_PARAMETER);
    SBK_TRY_C(sc_system_create_dsp(sbk::engine::system::get(), &config, dsp));
    SBK_TRY_C(sc_node_group_add_dsp(nodeGroup, *dsp, SC_DSP_INDEX_HEAD));
    return sbk::ok();
}

auto sbk::engine::node_instance_fsm::init_node_group(const event_init& init) -> sbk::result<void>
{
    ZoneScoped;
    sc_node_group* nodeGroup = nullptr;

    SBK_TRY_C(sc_system_create_node_group(sbk::engine::system::get(), &nodeGroup));
    m_nodeGroup.nodeGroup.reset(nodeGroup);
    SBK_TRYV(add_dsp_to_node_group(m_nodeGroup.nodeGroup.get(), &m_nodeGroup.lowpass, sc_dsp_config_init(SC_DSP_TYPE_LOWPASS)));
    SBK_TRYV(add_dsp_to_node_group(m_nodeGroup.nodeGroup.get(), &m_nodeGroup.highpass, sc_dsp_config_init(SC_DSP_TYPE_HIGHPASS)));

    for (const sbk::core::database_ptr<sbk::engine::effect_description>& desc : m_referencingNode->m_effectDescriptions)
    {
        if (const auto descShared = desc.shared())
        {
            sc_dsp* dsp = nullptr;
            (void)add_dsp_to_node_group(m_nodeGroup.nodeGroup.get(), &dsp, *descShared->get_config());

            int index = 0;
            for (const sbk::engine::effect_parameter_description& parameter : descShared->get_parameters())
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

    return sbk::ok();
}

auto sbk::engine::node_instance_fsm::init_parent() -> sbk::result<void>
{
    ZoneScoped;
    SBK_CHECK(m_referencingNode, SBK_ERR_NULL);
    SBK_CHECK(m_owner, SBK_ERR_NULL);

    std::shared_ptr<sbk::engine::node_base> nodeToReference;

    switch (m_referencingNode->get_node_status())
    {
        case node_status::top:
            nodeToReference = m_referencingNode->get_output_bus();
            SBK_CHECK_MSG(nodeToReference, SBK_ERR_BAKERY, "Output bus must be valid");
            break;
        case node_status::middle:
            nodeToReference = m_referencingNode->get_parent();
            SBK_CHECK_MSG(nodeToReference, SBK_ERR_BAKERY, "Parent must be valid");
            break;
        case node_status::null:
            nodeToReference = sbk::engine::system::get()->get_master_bus();
            SBK_CHECK_MSG(nodeToReference, SBK_ERR_BAKERY, "Master Bus invalid");
            break;
    }

    SBK_CHECK(nodeToReference, SBK_ERR_NULL);
    SBK_CHECK_MSG(nodeToReference->get_database_id() != m_referencingNode->get_database_id(), SBK_ERR_BAKERY, "Pointing to self. Cannot init parent");

    const event_init initData{.refNode = nodeToReference, .type = node_instance_type::bus, .m_owningGameObject = m_gameObject};
    SBK_TRY(m_parent, m_owner->create_runtime_object<sbk::engine::node_instance>());
    return m_parent->init(initData);
}

auto sbk::engine::node_instance_fsm::init_child() -> sbk::result<void>
{
    ZoneScoped;
    SBK_CHECK(m_referencingNode, SBK_ERR_NULL);
    SBK_CHECK(m_owner, SBK_ERR_NULL);

    if (const container* const container = m_referencingNode->try_convert_object<sbk::engine::container>())
    {
        gather_children_context context;
        context.numTimesPlayed = m_numTimesPlayed;
        context.parameters     = m_gameObject->get_local_parameters();

        container->gather_children_for_play(context);

        m_children.reserve(context.sounds.size());

        for (const auto& child : context.sounds)
        {
            SBK_CHECK(child, SBK_ERR_NULL);
            SBK_CHECK_MSG(child->get_database_id() != m_referencingNode->get_database_id(), SBK_ERR_BAKERY, "Referenced node was found in its child list. Self references should not happen");
            SBK_TRY(auto runtimeChild, m_owner->create_runtime_object<sbk::engine::node_instance>());
            m_children.push_back(runtimeChild);

            event_init childInit;
            childInit.parentForChildren  = m_owner;
            childInit.type               = node_instance_type::child;
            childInit.refNode            = child->get_database_id();
            childInit.m_owningGameObject = m_gameObject;

            SBK_TRYV(runtimeChild->init(childInit));
        }
    }

    return sbk::ok();
}

auto sbk::engine::node_instance_fsm::init_callbacks() -> void
{
    ZoneScoped;
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

auto sbk::engine::node_instance_fsm::set_volume(float oldVolume, float newVolume) -> void
{
    (void)oldVolume;

    if (m_nodeGroup.nodeGroup)
    {
        ma_sound_group_set_volume((ma_sound_group*)m_nodeGroup.nodeGroup->fader->state->userData, newVolume);
    }
}

auto sbk::engine::node_instance_fsm::set_pitch(float oldPitch, float newPitch) -> void
{
    (void)oldPitch;

    if (m_nodeGroup.nodeGroup)
    {
        ma_sound_group_set_pitch((ma_sound_group*)m_nodeGroup.nodeGroup->fader->state->userData, newPitch);
    }
}

auto sbk::engine::node_instance_fsm::set_lowpass(float oldLowpass, float newLowpass) -> void
{
    (void)oldLowpass;

    const double percentage    = sbk::maths::ease_out_cubic(newLowpass / 100.0);
    const double lowpassCutoff = (19980 - (19980.0 * percentage)) + 20.0;
    BOOST_ASSERT(lowpassCutoff >= 20.0);

    sc_dsp_set_parameter_float(m_nodeGroup.lowpass, SC_DSP_LOWPASS_CUTOFF, static_cast<float>(lowpassCutoff));
}

auto sbk::engine::node_instance_fsm::set_highpass(float oldHighpass, float newHighpass) -> void
{
    (void)oldHighpass;

    const double percentage     = sbk::maths::ease_in_cubic(newHighpass / 100.0);
    const double highpassCutoff = (19980.0 * percentage) + 20.0;
    BOOST_ASSERT(highpassCutoff >= 20.0);

    sc_dsp_set_parameter_float(m_nodeGroup.highpass, SC_DSP_HIGHPASS_CUTOFF, static_cast<float>(highpassCutoff));
}
