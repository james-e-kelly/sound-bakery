#include "gameobject.h"

#include "sound_bakery/event/event.h"
#include "sound_bakery/node/container/container.h"
#include "sound_bakery/voice/voice.h"

using namespace sbk::engine;

DEFINE_REFLECTION(sbk::engine::GameObject)

Voice* GameObject::playContainer(Container* container)
{
    if (container)
    {
        std::unique_ptr<Voice>& voice = m_voices.emplace_back(std::make_unique<Voice>(this));
        voice->playContainer(container);
        return voice.get();
    }
    else
    {
        return nullptr;
    }
}

void sbk::engine::GameObject::postEvent(Event* event)
{
    if (event)
    {
        for (const Action& action : event->m_actions)
        {
            sbk::engine::Container* container   = nullptr;
            sbk::engine::Event* childEvent      = nullptr;
            sbk::engine::GameObject* gameObject = nullptr;

            if (const sbk::core::DatabasePtr<sbk::core::database_object>& destination = action.m_destination;
                destination.lookup())
            {
                container  = destination->try_convert_object<sbk::engine::Container>();
                childEvent = destination->try_convert_object<sbk::engine::Event>();
                gameObject = destination->try_convert_object<sbk::engine::GameObject>();
            }

            switch (action.m_type)
            {
                case SB_ACTION_PLAY:
                    if (container)
                    {
                        playContainer(container);
                    }
                    else if (childEvent)
                    {
                        postEvent(childEvent);
                    }
                    break;
                case SB_ACTION_STOP:
                    if (container)
                    {
                        stopContainer(container);
                    }
                    else if (childEvent)
                    {
                    }
                    else if (gameObject)
                    {
                        gameObject->stopAll();
                    }
                    break;
                default:
                    break;
            }
        }
    }
}

void sbk::engine::GameObject::stopVoice(Voice* voice)
{
    for (std::size_t i = m_voices.size(); i--;)
    {
        if (m_voices[i].get() == voice)
        {
            m_voices.erase(m_voices.begin() + i);
            break;
        }
    }
}

void sbk::engine::GameObject::stopContainer(Container* container)
{
    for (std::size_t i = m_voices.size(); i--;)
    {
        if (const std::unique_ptr<Voice>& voice = m_voices[i])
        {
            if (voice->playingContainer(container))
            {
                m_voices.erase(m_voices.begin() + i);
                break;
            }
        }
    }
}

void GameObject::stopAll() { m_voices.clear(); }

void GameObject::update()
{
    if (m_voices.size())
    {
        std::vector<std::unique_ptr<Voice>>::iterator iter;
        for (iter = m_voices.begin(); iter != m_voices.end();)
        {
            iter->get()->update();

            if (!iter->get()->isPlaying())
            {
                iter = m_voices.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
    }
}

bool sbk::engine::GameObject::isPlaying() const noexcept { return voiceCount(); }

std::size_t sbk::engine::GameObject::voiceCount() const noexcept { return m_voices.size(); }

Voice* sbk::engine::GameObject::getVoice(std::size_t index) const
{
    if (index >= 0 && index < m_voices.size())
    {
        return m_voices[index].get();
    }
    return nullptr;
}
