#include "voice.h"

#include "sound_bakery/gameobject/gameobject.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/sound/sound.h"
#include "sound_bakery/voice/node_instance.h"

using namespace sbk::engine;

void sbk::engine::voice::play_container(container* container)
{
    destroy_all();

    m_playingContainer = container;

    const std::shared_ptr<node_instance> voiceInstance = create_runtime_object<node_instance>();

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
        destroy_all();
    }
}

void voice::update()
{
    if (!get_objects().empty())
    {
        std::vector<std::shared_ptr<sbk::core::object>>::iterator iter;
        for (iter = get_objects().begin(); iter != get_objects().end();)
        {
            if (sbk::engine::node_instance* const nodeInstance =
                    iter->get()->try_convert_object<sbk::engine::node_instance>())
            {
                nodeInstance->update();

                if (!nodeInstance->isPlaying())
                {
                    iter = get_objects().erase(iter);
                }
                else
                {
                    ++iter;
                }
            }
        }
    }
}

bool sbk::engine::voice::playing_container(container* container) const noexcept
{
    if (container == nullptr)
    {
        return false;
    }

    if (container->get_database_id() == m_playingContainer.id())
    {
        return true;
    }

    auto containerEqual = [id = container->get_database_id()](const std::shared_ptr<sbk::core::object>& object)
    {
        if (!object)
        {
            return false;
        }

        std::shared_ptr<sbk::engine::node_instance> nodeInstance =
            std::static_pointer_cast<sbk::engine::node_instance>(object);

        if (!nodeInstance)
        {
            return false;
        }

        if (nodeInstance->getReferencingNode()->get_database_id() == id)
        {
            return true;
        }

        node_instance* parentNodeInstance = nodeInstance->get_parent();

        while (parentNodeInstance)
        {
            if (parentNodeInstance->getReferencingNode()->get_database_id() == id)
            {
                return true;
            }

            parentNodeInstance = parentNodeInstance->get_parent();
        }

        return false;
    };

    return std::find_if(get_objects().begin(), get_objects().end(), containerEqual) != get_objects().end();
}

const std::vector<std::shared_ptr<node_instance>> sbk::engine::voice::get_voices() const noexcept
{
    std::vector<std::shared_ptr<node_instance>> nodeInstances;

    for (std::size_t index = 0; index < get_objects().size(); ++index)
    {
        if (get_objects()[index])
        {
            if (std::shared_ptr<sbk::engine::node_instance> nodeInstance = std::static_pointer_cast<sbk::engine::node_instance>(get_objects()[index]))
            {
                nodeInstances.push_back(nodeInstance);
            }
        }
    }

    return nodeInstances;
}

std::size_t sbk::engine::voice::num_voices() const 
{
    // Just assuming all owned objects are node instances
    return get_objects().size(); 
}

node_instance* sbk::engine::voice::node_instance_at(std::size_t index) const { return get_objects()[index]->try_convert_object<sbk::engine::node_instance>(); }

bool sbk::engine::voice::is_playing() const { return !get_objects().empty(); }
