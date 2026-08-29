#pragma once

#include "sound_bakery/core/object/object.h"
#include "sound_bakery/core/property.h"

namespace sbk::engine
{
    class container;
    class node;
    class game_object;

    struct container_instance
    {
        sbk_id containerReference{};
        sc_voice_handle voiceHandle{};
        unsigned int childCount{};
        std::size_t parentIndex{};
        bool finished{};
    };

    struct property_subscription
    {
        sbk::core::float_property::property_changed_delegate* delegate;
        DelegateHandle handle;
    };

    struct voice_property_watch
    {
        ~voice_property_watch();

        sc_voice_handle voiceHandle{};
        eastl::vector<std::shared_ptr<node>> nodeChain;
        eastl::vector<property_subscription> subscriptions;
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