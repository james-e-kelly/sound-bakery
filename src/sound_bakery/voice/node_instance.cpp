#include "node_instance.h"

#include "sound_bakery/maths/easing.h"
#include "sound_bakery/node/bus/bus.h"
#include "sound_bakery/node/container/sequence_container.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/sound/sound.h"
#include "sound_bakery/system.h"
#include "sound_bakery/voice/voice.h"

using namespace sbk::engine;

DEFINE_REFLECTION(sbk::engine::NodeInstance)

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

bool sbk::engine::NodeInstance::init(const InitNodeInstance& initData)
{
    if (m_state != NodeInstanceState::UNINIT)
    {
        return true;
    }

    if (!initData.refNode.lookup())
    {
        return false;
    }

    bool converted = false;
    m_referencingNode =
        rttr::wrapper_mapper<sbk::core::DatabasePtr<NodeBase>>::convert<Node>(initData.refNode, converted).shared();

    if (!m_nodeGroup.initNodeGroup(*initData.refNode.raw()))
    {
        return false;
    }

    m_owningVoice = initData.owningVoice;

    m_referencingNode->m_volume.getDelegate().AddRaw(this, &NodeInstance::setVolume);
    m_referencingNode->m_pitch.getDelegate().AddRaw(this, &NodeInstance::setPitch);
    m_referencingNode->m_lowpass.getDelegate().AddRaw(this, &NodeInstance::setLowpass);
    m_referencingNode->m_highpass.getDelegate().AddRaw(this, &NodeInstance::setHighpass);

    setVolume(0.0F, m_referencingNode->m_volume.get());
    setPitch(0.0F, m_referencingNode->m_pitch.get());
    setLowpass(0.0F, m_referencingNode->m_lowpass.get());
    setHighpass(0.0F, m_referencingNode->m_highpass.get());

    bool success = false;

    switch (initData.type)
    {
        case NodeInstanceType::CHILD:
        {
            if (initData.refNode->getChildCount() == 0)
            {
                success = true;
            }
            else
            {
                success = m_children.createChildren(*initData.refNode.raw(), m_owningVoice, this, ++m_numTimesPlayed);
            }

            break;
        }
        case NodeInstanceType::BUS:
        {
            if (const Bus* const bus = initData.refNode->try_convert_object<Bus>())
            {
                // Checks nullptr as master busses are technically busses without an output, even if they're not marked
                // as masters
                if (bus->isMasterBus() || bus->parent() == nullptr)
                {
                    success = true;
                    break;
                }
            }

            success = m_parent.createParent(*initData.refNode.raw(), m_owningVoice);
            break;
        }
        case NodeInstanceType::MAIN:
        {
            success = m_parent.createParent(*initData.refNode.raw(), m_owningVoice);
            success &= m_children.createChildren(*initData.refNode.raw(), m_owningVoice, this, ++m_numTimesPlayed);
            break;
        }
    }

    if (m_parent.parent)
    {
        sc_node_group_set_parent(m_nodeGroup.nodeGroup.get(), m_parent.parent->getBus());
    }
    else if (initData.parentForChildren)
    {
        sc_node_group_set_parent(m_nodeGroup.nodeGroup.get(), initData.parentForChildren->getBus());
    }
    // else, should be connected to master bus by default

    if (success)
    {
        m_state = NodeInstanceState::STOPPED;
    }

    return success;
}

bool NodeInstance::play()
{
    if (isPlaying())
    {
        return true;
    }

    if (m_referencingNode->getType() == rttr::type::get<SoundContainer>())
    {
        SoundContainer* soundContainer   = m_referencingNode->try_convert_object<SoundContainer>();
        Sound* engineSound               = soundContainer->getSound();
        sc_sound* sound                  = engineSound != nullptr ? engineSound->getSound() : nullptr;
        sc_sound_instance* soundInstance = nullptr;

        sc_result playSoundResult =
            sc_system_play_sound(getChef(), sound, &soundInstance, m_nodeGroup.nodeGroup.get(), MA_FALSE);

        if (playSoundResult == MA_SUCCESS)
        {
            m_state = NodeInstanceState::PLAYING;
            m_soundInstance.reset(soundInstance);
        }
    }
    else
    {
        unsigned int playingCount = 0;

        for (const auto& child : m_children.childrenNodes)
        {
            if (child->play())
            {
                ++playingCount;
            }
        }

        if (playingCount > 0)
        {
            m_state = NodeInstanceState::PLAYING;
        }
    }

    return isPlaying();
}

void NodeInstance::update()
{
    if (m_soundInstance)
    {
        if (ma_sound_at_end(&m_soundInstance->sound) == MA_TRUE)
        {
            m_state = NodeInstanceState::STOPPED;
            m_soundInstance.release();
        }
    }
    else if (!m_children.childrenNodes.empty())
    {
        unsigned int stoppedChildren = 0;

        for (const auto& child : m_children.childrenNodes)
        {
            child->update();

            if (!child->isPlaying())
            {
                ++stoppedChildren;
            }
        }

        if (stoppedChildren == m_children.childrenNodes.size())
        {
            m_state = NodeInstanceState::STOPPED;
            m_children.childrenNodes.clear();

            // Sequence nodes retrigger when the current sound stops
            if (m_referencingNode->getType() == rttr::type::get<SequenceContainer>())
            {
                m_children.createChildren(*m_referencingNode->try_convert_object<NodeBase>(), m_owningVoice, this,
                                          ++m_numTimesPlayed);
                play();
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////

void NodeInstance::setVolume(float oldVolume, float newVolume)
{
    (void)oldVolume;

    if (m_nodeGroup.nodeGroup)
    {
        ma_sound_group_set_volume((ma_sound_group*)m_nodeGroup.nodeGroup->fader->state->userData, newVolume);
    }
}

void NodeInstance::setPitch(float oldPitch, float newPitch)
{
    (void)oldPitch;

    if (m_nodeGroup.nodeGroup)
    {
        ma_sound_group_set_pitch((ma_sound_group*)m_nodeGroup.nodeGroup->fader->state->userData, newPitch);
    }
}

void NodeInstance::setLowpass(float oldLowpass, float newLowpass)
{
    (void)oldLowpass;

    const double percentage    = sbk::Maths::easeOutCubic(newLowpass / 100.0);
    const double lowpassCutoff = (19980 - (19980.0 * percentage)) + 20.0;
    assert(lowpassCutoff >= 20.0);

    sc_dsp_set_parameter_float(m_nodeGroup.lowpass, SC_DSP_LOWPASS_CUTOFF, static_cast<float>(lowpassCutoff));
}

void NodeInstance::setHighpass(float oldHighpass, float newHighpass)
{
    (void)oldHighpass;

    const double percentage     = sbk::Maths::easeInCubic(newHighpass / 100.0);
    const double highpassCutoff = (19980.0 * percentage) + 20.0;
    assert(highpassCutoff >= 20.0);

    sc_dsp_set_parameter_float(m_nodeGroup.highpass, SC_DSP_HIGHPASS_CUTOFF, static_cast<float>(highpassCutoff));
}
