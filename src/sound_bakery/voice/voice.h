#pragma once

#include "sound_bakery/core/object/object.h"
#include "sound_bakery/core/property.h"

namespace sbk::engine
{
    class container;
    class node;
    class game_object;

    /**
     * @brief A runtime, playing representation of a @ref sbk::engine::container.
     * 
     * If the container is playing a sound, it has a voice handle.
     * If the container is playing child sounds, it has a child count.
     * 
     * All nodes know their parent index (index into the voice's container array).
     * Once an instance ends, it can tell its parent that it's finished, decrementing its child count.
     * 
     * Instances are finished when their sc_voice_handle has ended or its child count is 0.
     */
    struct container_instance
    {
        sbk_id containerReference{};    //< The ID of the container this instance references
        sc_voice_handle voiceHandle{};  //< The potentially playing voice handle
        unsigned int childCount{};      //< Number of children playing from this instance. Can be zero if the voice has finished or we're playing a sound and @ref is not 0
        std::size_t parentIndex{};      //< Parent index. Can be used to tell the parent we have finished or to retrigger sounds
        bool finished{};                //< Whether this instance has finished playing and can be stopped and cleaned up
    };

    struct property_subscription
    {
        property_subscription() = default;
        property_subscription(property_subscription&& other) noexcept = default;
        property_subscription& operator=(property_subscription&& other) noexcept = default;
        property_subscription(const property_subscription&) = delete;
        property_subscription& operator=(const property_subscription&) = delete;
        ~property_subscription();

        sbk::core::float_property::property_changed_delegate* delegate{};  //< Raw pointer but the owning voice_property_watch holds a shared ptr to the node and its properties so should stay valid
        DelegateHandle handle;
    };

    struct voice_dsp_instance
    {
        sc_dsp* dsp{};
        sc_uint32 parameterIndex{};
        property_subscription subscription;
    };

    struct voice_property_watch
    {
        sc_voice_handle voiceHandle{};
        eastl::vector<std::shared_ptr<node>> nodeChain;
        eastl::vector<property_subscription> subscriptions;
        eastl::vector<voice_dsp_instance> dspInstances;
    };

    /**
     * @brief A runtime graph of nodes and busses, playing a sound or many.
     */
    class SB_CLASS voice : public sbk::core::object
    {
        REGISTER_REFLECTION(voice, sbk::core::object)

    public:
        auto play_container(container* container) -> sbk::result<void>;

        auto update() -> void;

        [[nodiscard]] auto playing_container(container* container) const noexcept -> bool;
        [[nodiscard]] auto is_playing() const -> bool;
        [[nodiscard]] auto get_owning_game_object() const -> game_object*;

        [[nodiscard]] auto get_instances() const -> const eastl::vector<container_instance>& { return m_instances; }
        [[nodiscard]] auto get_output_busses() const -> const eastl::vector<std::shared_ptr<sc_node_group>>& { return m_outputBusses; }

    private:
        auto subscribe_to_properties(sc_voice_handle handle, const std::shared_ptr<container>& leaf) -> void;
        auto recompute_voice_dsp(sc_voice_handle handle) -> sbk::result<>;

        eastl::vector<container_instance> m_instances;
        eastl::vector<std::shared_ptr<sc_node_group>> m_outputBusses;
        eastl::vector<voice_property_watch> m_propertyWatches;
    };
}  // namespace sbk::engine