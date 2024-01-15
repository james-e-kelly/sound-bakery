#include "voice.h"

#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/sound/sound.h"
#include "sound_bakery/voice/node_instance.h"

using namespace SB::Engine;

Voice::Voice()  = default;
Voice::~Voice() = default;

void SB::Engine::Voice::playContainer(Container* container) noexcept
{
    m_voiceInstances.clear();

    m_playingContainer = container;

    std::vector<Container*> containersToPlay;

    if (container->getType() == rttr::type::get<SoundContainer>())
    {
        containersToPlay.push_back(container);
    }
    else
    {
        containersToPlay.reserve(3);
        container->gatherSounds(
            containersToPlay, SB::Engine::Container::RuntimeFloatParameterMap(),
            SB::Engine::Container::RuntimeIntParameterMap());
    }

    for (Container* const containerToPlay : containersToPlay)
    {
        if (!containerToPlay)
        {
            continue;
        }

        if (SoundContainer* const soundContainer =
                containerToPlay->tryConvertObject<SoundContainer>())
        {
            if (Sound* const sound = soundContainer->getSound())
            {
                std::unique_ptr<NodeInstance>& voiceInstance =
                    m_voiceInstances.emplace_back(
                        std::make_unique<NodeInstance>());
                voiceInstance->setSoundInstance(soundContainer, sound);
            }
        }
        else
        {
            // error
        }
    }
}

void Voice::update()
{
    if (m_voiceInstances.size())
    {
        std::vector<std::unique_ptr<NodeInstance>>::iterator iter;
        for (iter = m_voiceInstances.begin(); iter != m_voiceInstances.end();)
        {
            if (!iter->get()->isPlaying())
            {
                iter = m_voiceInstances.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
    }
}

bool SB::Engine::Voice::playingContainer(Container* container) const noexcept
{
    if (!container)
    {
        return false;
    }

    if (container->getDatabaseID() == m_playingContainer.id())
    {
        return true;
    }

    auto containerEqual = [id = container->getDatabaseID()](
                              const std::unique_ptr<NodeInstance>& node)
    {
        if (!node)
        {
            return false;
        }

        const NodeInstance* nodeInstance = node.get();

        if (nodeInstance->getReferencingNode()->getDatabaseID() == id)
        {
            return true;
        }

        std::shared_ptr<NodeInstance> sharedNodeInstance =
            nodeInstance->getParent().lock();

        while (sharedNodeInstance)
        {
            if (sharedNodeInstance->getReferencingNode()->getDatabaseID() == id)
            {
                return true;
            }

            sharedNodeInstance = sharedNodeInstance->getParent().lock();
        }

        return false;
    };

    return std::find_if(m_voiceInstances.begin(), m_voiceInstances.end(),
                        containerEqual) != m_voiceInstances.end();
}

const std::vector<std::unique_ptr<NodeInstance>>& SB::Engine::Voice::getVoices()
    const noexcept
{
    return m_voiceInstances;
}

std::size_t SB::Engine::Voice::voices() const
{
    return m_voiceInstances.size();
}

NodeInstance* SB::Engine::Voice::voice(std::size_t index) const
{
    return m_voiceInstances[index].get();
}

bool SB::Engine::Voice::isPlaying() const { return m_voiceInstances.size(); }
