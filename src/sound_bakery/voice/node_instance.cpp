#include "node_instance.h"

#include "sound_bakery/maths/easing.h"
#include "sound_bakery/node/bus/bus.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/sound/sound.h"
#include "sound_bakery/system.h"
#include "sound_bakery/voice/voice.h"

using namespace SB::Engine;

NodeInstance::NodeInstance() = default;

NodeInstance::~NodeInstance()
{
    if (m_referencingNode.valid())
    {
        m_referencingNode->m_volume.getDelegate().RemoveObject(this);
        m_referencingNode->m_pitch.getDelegate().RemoveObject(this);
        m_referencingNode->m_lowpass.getDelegate().RemoveObject(this);
        m_referencingNode->m_highpass.getDelegate().RemoveObject(this);
    }
}

void SB::Engine::NodeInstance::setSoundInstance(SoundContainer* soundContainer,
                                                Sound* sound) noexcept
{
    m_referencingNode      = soundContainer;
    m_referencingSoundNode = soundContainer;

    init();

    ma_result attach = ma_node_attach_output_bus(
        sound->getSound(), 0, m_nodeGroup.get()->m_tail->m_state->m_userData,
        0);
    assert(attach == MA_SUCCESS);

    ma_result start = ma_sound_start(sound->getSound());
    assert(start == MA_SUCCESS);
}

void SB::Engine::NodeInstance::setNodeInstance(Container* container) noexcept
{
    m_referencingNode = container;

    init();
}

void SB::Engine::NodeInstance::setBusInstance(Bus* bus) noexcept
{
    m_referencingNode = bus;
    m_referencingBus  = bus;

    init();
}

void SB::Engine::NodeInstance::createParentBusInstance()
{
    if (NodeBase* outputBusNode = m_referencingNode->outputBus())
    {
        m_parent = outputBusNode->tryConvertObject<Bus>()->lockAndCopy();
    }
    else
    {
        createMasterBusParent();
    }
}

void SB::Engine::NodeInstance::createParentInstance()
{
    Container* const parent =
        m_referencingNode->parent()->tryConvertObject<Container>();

    m_parent = std::make_shared<NodeInstance>();
    m_parent->setNodeInstance(parent);
}

void SB::Engine::NodeInstance::init()
{
    createChannelGroup();
    createDSP();
    bindDelegates();
    createParent();
    attachToParent();
}

void NodeInstance::createChannelGroup()
{
    SC_NODE_GROUP* nodeGroup = nullptr;
    SC_System_CreateNodeGroup(getChef(), &nodeGroup);

    m_nodeGroup.reset(nodeGroup);
}

void SB::Engine::NodeInstance::createParent()
{
    switch (m_referencingNode->getNodeStatus())
    {
        case SB_NODE_NULL:
            createMasterBusParent();
            break;
        case SB_NODE_MIDDLE:
            createParentInstance();
            break;
        case SB_NODE_TOP:
            createParentBusInstance();
            break;
    }
}

void SB::Engine::NodeInstance::createMasterBusParent()
{
    const SB::Core::DatabasePtr<Bus>& masterBus =
        SB::Engine::System::get()->getMasterBus();

    if (masterBus.lookup() && masterBus.id() != m_referencingNode.id())
    {
        m_parent = masterBus->lockAndCopy();
    }
}

void SB::Engine::NodeInstance::bindDelegates()
{
    m_referencingNode->m_volume.getDelegate().AddRaw(this,
                                                     &NodeInstance::setVolume);
    m_referencingNode->m_pitch.getDelegate().AddRaw(this,
                                                    &NodeInstance::setPitch);
    m_referencingNode->m_lowpass.getDelegate().AddRaw(
        this, &NodeInstance::setLowpass);
    m_referencingNode->m_highpass.getDelegate().AddRaw(
        this, &NodeInstance::setHighpass);

    setVolume(0.0F, m_referencingNode->m_volume.get());
    setPitch(0.0F, m_referencingNode->m_pitch.get());
    setLowpass(0.0F, m_referencingNode->m_lowpass.get());
    setHighpass(0.0F, m_referencingNode->m_highpass.get());
}

void SB::Engine::NodeInstance::createDSP()
{
    const SC_DSP_CONFIG lpfConfig = SC_DSP_Config_Init(SC_DSP_TYPE_LOWPASS);
    SC_System_CreateDSP(getChef(), &lpfConfig, &m_lowpass);
    SC_NodeGroup_AddDSP(m_nodeGroup.get(), m_lowpass, SC_DSP_INDEX_HEAD);

    const SC_DSP_CONFIG hpfConfig = SC_DSP_Config_Init(SC_DSP_TYPE_HIGHPASS);
    SC_System_CreateDSP(getChef(), &hpfConfig, &m_highpass);
    SC_NodeGroup_AddDSP(m_nodeGroup.get(), m_highpass, SC_DSP_INDEX_HEAD);

    for (const SB::Core::DatabasePtr<SB::Engine::EffectDescription>& desc :
         m_referencingNode->m_effectDescriptions)
    {
        if (desc.lookup())
        {
            SC_DSP* dsp = nullptr;

            SC_RESULT create =
                SC_System_CreateDSP(getChef(), desc->getConfig(), &dsp);
            assert(create == MA_SUCCESS);

            SC_RESULT add =
                SC_NodeGroup_AddDSP(m_nodeGroup.get(), dsp, SC_DSP_INDEX_HEAD);
            assert(add == MA_SUCCESS);

            int index = 0;
            for (const SB::Engine::EffectParameterDescription& parameter :
                 desc->getParameters())
            {
                switch (parameter.m_parameter.m_type)
                {
                    case SC_DSP_PARAMETER_TYPE_FLOAT:
                        SC_DSP_SetParameterFloat(
                            dsp, index++,
                            parameter.m_parameter.m_float.m_value);
                        break;
                }
            }
        }
    }
}

void SB::Engine::NodeInstance::attachToParent()
{
    if (m_parent)
    {
        SC_NodeGroup_SetParent(m_nodeGroup.get(), m_parent->getBus());
    }
}

bool SB::Engine::NodeInstance::isPlaying() const
{
    if (SB::Engine::SoundContainer* const soundContainer =
            m_referencingNode->tryConvertObject<SB::Engine::SoundContainer>())
    {
        return static_cast<bool>(ma_sound_is_playing(soundContainer->getSound()->getSound()));
    }
    return false;
}

void NodeInstance::setVolume(float oldVolume, float newVolume)
{
    if (m_nodeGroup)
    {
        ma_sound_group_set_volume(
            (ma_sound_group*)m_nodeGroup->m_fader->m_state->m_userData,
            newVolume);
    }
}

void NodeInstance::setPitch(float oldPitch, float newPitch)
{
    if (m_nodeGroup)
    {
        ma_sound_group_set_pitch(
            (ma_sound_group*)m_nodeGroup->m_fader->m_state->m_userData,
            newPitch);
    }
}

void NodeInstance::setLowpass(float oldLowpass, float newLowpass)
{
    (void)oldLowpass;

    const double percentage    = SB::Maths::easeOutCubic(newLowpass / 100.0);
    const double lowpassCutoff = (19980 - (19980.0 * percentage)) + 20.0;
    assert(lowpassCutoff >= 20.0);

    SC_DSP_SetParameterFloat(m_lowpass, SC_DSP_LOWPASS_CUTOFF,
                             static_cast<float>(lowpassCutoff));
}

void NodeInstance::setHighpass(float oldHighpass, float newHighpass)
{
    (void)oldHighpass;

    const double percentage     = SB::Maths::easeInCubic(newHighpass / 100.0);
    const double highpassCutoff = (19980.0 * percentage) + 20.0;
    assert(highpassCutoff >= 20.0);

    SC_DSP_SetParameterFloat(m_highpass, SC_DSP_HIGHPASS_CUTOFF,
                             static_cast<float>(highpassCutoff));
}