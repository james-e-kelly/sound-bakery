#include "voice.h"

#include "sound_bakery/gameobject/gameobject.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/sound/sound.h"
#include "sound_bakery/voice/node_instance.h"

using namespace sbk::engine;

void sbk::engine::voice::play_container(Container* container)
{
    m_voiceInstances.clear();

    m_playingContainer = container;

    const std::unique_ptr<node_instance>& voiceInstance =
        m_voiceInstances.emplace_back(std::make_unique<node_instance>());

    InitNodeInstance initData;
    initData.refNode     = container->try_convert_object<node_base>();
    initData.type        = sbk::engine::NodeInstanceType::MAIN;
    initData.owningVoice = this;

    if (voiceInstance->init(initData))
    {
        voiceInstance->play();
    }
    else
    {
        m_voiceInstances.clear();
    }
}

void voice::update()
{
    if (!m_voiceInstances.empty())
    {
        std::vector<std::unique_ptr<node_instance>>::iterator iter;
        for (iter = m_voiceInstances.begin(); iter != m_voiceInstances.end();)
        {
            iter->get()->update();

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

bool sbk::engine::voice::playing_container(Container* container) const noexcept
{
    if (container == nullptr)
    {
        return false;
    }

    if (container->get_database_id() == m_playingContainer.id())
    {
        return true;
    }

    auto containerEqual = [id = container->get_database_id()](const std::unique_ptr<node_instance>& node)
    {
        if (!node)
        {
            return false;
        }

        const node_instance* nodeInstance = node.get();

        if (nodeInstance->getReferencingNode()->get_database_id() == id)
        {
            return true;
        }

        node_instance* sharedNodeInstance = nodeInstance->getParent();

        while (sharedNodeInstance)
        {
            if (sharedNodeInstance->getReferencingNode()->get_database_id() == id)
            {
                return true;
            }

            sharedNodeInstance = sharedNodeInstance->getParent();
        }

        return false;
    };

    return std::find_if(m_voiceInstances.begin(), m_voiceInstances.end(), containerEqual) != m_voiceInstances.end();
}

const std::vector<std::unique_ptr<node_instance>>& sbk::engine::voice::get_voices() const noexcept
{
    return m_voiceInstances;
}

std::size_t sbk::engine::voice::num_voices() const { return m_voiceInstances.size(); }

node_instance* sbk::engine::voice::node_instance_at(std::size_t index) const { return m_voiceInstances[index].get(); }

bool sbk::engine::voice::is_playing() const { return !m_voiceInstances.empty(); }
