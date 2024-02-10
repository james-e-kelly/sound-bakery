#include "node_instance.h"

#include "sound_bakery/maths/easing.h"
#include "sound_bakery/node/bus/bus.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/sound/sound.h"
#include "sound_bakery/system.h"
#include "sound_bakery/voice/voice.h"

using namespace SB::Engine;

NodeInstance::~NodeInstance()
{
    if (m_referencingNode != nullptr)
    {
        m_referencingNode->m_volume.getDelegate().RemoveObject(this);
        m_referencingNode->m_pitch.getDelegate().RemoveObject(this);
        m_referencingNode->m_lowpass.getDelegate().RemoveObject(this);
        m_referencingNode->m_highpass.getDelegate().RemoveObject(this);
    }
}

bool SB::Engine::NodeInstance::init(const SB::Core::DatabasePtr<NodeBase>& refNode, const NodeInstanceType type)
{ 
    if (m_state != NodeInstanceState::UNINIT)
    {
        return true;
    }

    if (!refNode.lookup())
    {
        return false;
    }

    if (!m_nodeGroup.initNodeGroup(*refNode.raw()))
    {
        return false;
    }

    m_referencingNode->m_volume.getDelegate().AddRaw(this, &NodeInstance::setVolume);
    m_referencingNode->m_pitch.getDelegate().AddRaw(this, &NodeInstance::setPitch);
    m_referencingNode->m_lowpass.getDelegate().AddRaw(this, &NodeInstance::setLowpass);
    m_referencingNode->m_highpass.getDelegate().AddRaw(this, &NodeInstance::setHighpass);

    setVolume(0.0F, m_referencingNode->m_volume.get());
    setPitch(0.0F, m_referencingNode->m_pitch.get());
    setLowpass(0.0F, m_referencingNode->m_lowpass.get());
    setHighpass(0.0F, m_referencingNode->m_highpass.get());

    bool success = false;

    switch (type)
    {
        case NodeInstanceType::CHILD:
        {
            success = m_children.createChildren(*refNode.raw());
            break;
        }
        case NodeInstanceType::BUS:
        case NodeInstanceType::MAIN:
        {
            success = m_parent.createParent(*refNode.raw());
            break;
        }
    }

    if (success)
    {
        if (m_parent.parent)
        {
            SC_NodeGroup_SetParent(m_nodeGroup.nodeGroup.get(), m_parent.parent->getBus());
        }

        m_state = NodeInstanceState::STOPPED;
    }

    return success;
}

bool NodeInstance::play() 
{
    if (m_state == NodeInstanceState::PLAYING ||
        m_state == NodeInstanceState::STOPPING)
    {
        return true;
    }

    if (m_referencingNode->getType() == rttr::type::get<SoundContainer>())
    {
        SoundContainer* soundContainer = m_referencingNode->tryConvertObject<SoundContainer>();
        Sound* engineSound = soundContainer->getSound();
        SC_SOUND* sound = engineSound ? engineSound->getSound() : nullptr;
        SC_SOUND_INSTANCE* soundInstance = nullptr;

        SC_System_PlaySound(getChef(), sound, &soundInstance, m_nodeGroup.nodeGroup.get(), MA_FALSE);
    }
    else
    {
        m_children.createChildren(*m_referencingNode->tryConvertObject<NodeBase>());

        for (const auto& child : m_children.childrenNodes)
        {
            child->play();
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////////////

void NodeInstance::setVolume(float oldVolume, float newVolume)
{
    (void)oldVolume;

    if (m_nodeGroup.nodeGroup)
    {
        ma_sound_group_set_volume((ma_sound_group*)m_nodeGroup.nodeGroup->m_fader->m_state->m_userData, newVolume);
    }
}

void NodeInstance::setPitch(float oldPitch, float newPitch)
{
    (void)oldPitch;

    if (m_nodeGroup.nodeGroup)
    {
        ma_sound_group_set_pitch((ma_sound_group*)m_nodeGroup.nodeGroup->m_fader->m_state->m_userData, newPitch);
    }
}

void NodeInstance::setLowpass(float oldLowpass, float newLowpass)
{
    (void)oldLowpass;

    const double percentage    = SB::Maths::easeOutCubic(newLowpass / 100.0);
    const double lowpassCutoff = (19980 - (19980.0 * percentage)) + 20.0;
    assert(lowpassCutoff >= 20.0);

    SC_DSP_SetParameterFloat(m_nodeGroup.lowpass, SC_DSP_LOWPASS_CUTOFF, static_cast<float>(lowpassCutoff));
}

void NodeInstance::setHighpass(float oldHighpass, float newHighpass)
{
    (void)oldHighpass;

    const double percentage     = SB::Maths::easeInCubic(newHighpass / 100.0);
    const double highpassCutoff = (19980.0 * percentage) + 20.0;
    assert(highpassCutoff >= 20.0);

    SC_DSP_SetParameterFloat(m_nodeGroup.highpass, SC_DSP_HIGHPASS_CUTOFF, static_cast<float>(highpassCutoff));
}
