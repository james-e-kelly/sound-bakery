#include "voice.h"

#include "sound_bakery/gameobject/gameobject.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/sound/sound.h"
#include "sound_bakery/voice/node_instance.h"

using namespace sbk::engine;

DEFINE_REFLECTION(voice)

void sbk::engine::voice::play_container(container* container)
{
    ZoneScoped;
    remove_all();

    m_playingContainer = container;

    const std::shared_ptr<node_instance> voiceInstance = create_runtime_object<node_instance>().get();

    event_init initData;
    initData.refNode     = container->try_convert_object<node_base>();
    initData.type        = sbk::engine::node_instance_type::main;
    initData.m_owningGameObject = get_owning_game_object();

    if (voiceInstance->init(initData) == MA_SUCCESS)
    {
        voiceInstance->play();
    }
    else
    {
        remove_all();
    }
}

void voice::update()
{
    ZoneScoped;

    iterate_referenced_objects([&](std::shared_ptr<object> object)
        {
            if (std::shared_ptr<sbk::engine::node_instance> nodeInstance = std::static_pointer_cast<sbk::engine::node_instance>(object))
            {
                nodeInstance->update();

                if (nodeInstance->is_stopped())
                {
                    return sbk::core::object_iterate_action::destroy_and_next;
                }
            }
            return sbk::core::object_iterate_action::next;
        });
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

        if (nodeInstance->get_referencing_node()->get_database_id() == id)
        {
            return true;
        }

        node_instance* parentNodeInstance = nodeInstance->get_parent();

        while (parentNodeInstance)
        {
            if (parentNodeInstance->get_referencing_node()->get_database_id() == id)
            {
                return true;
            }

            parentNodeInstance = parentNodeInstance->get_parent();
        }

        return false;
    };

    return referenced_object_exists_predicate(containerEqual).get();
}

const std::vector<std::shared_ptr<node_instance>> sbk::engine::voice::get_voices() const noexcept
{
    std::vector<std::shared_ptr<node_instance>> nodeInstances;

    iterate_const_referenced_objects([&](std::shared_ptr<object> object) 
        {
            if (std::shared_ptr<sbk::engine::node_instance> nodeInstance = std::static_pointer_cast<sbk::engine::node_instance>(object))
            {
                nodeInstances.push_back(nodeInstance);
            }
            return sbk::core::object_iterate_action::next;
        });

    return nodeInstances;
}

std::size_t sbk::engine::voice::num_voices() const
{
    // Just assuming all owned objects are node instances
    return get_referenced_objects_size().get();
}

node_instance* sbk::engine::voice::node_instance_at(std::size_t index) const
{
    return get_referenced_object_at(index).get()->try_convert_object<sbk::engine::node_instance>();
}

bool sbk::engine::voice::is_playing() const { return get_referenced_objects_size().get(); }

game_object* sbk::engine::voice::get_owning_game_object() const
{
    return static_cast<sbk::core::object*>(get_owner())->try_convert_object<sbk::engine::game_object>();
}
