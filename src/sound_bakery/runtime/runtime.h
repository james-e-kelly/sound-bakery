#pragma once

#include "sound_bakery/pch.h"
#include "sound_bakery/core/object/object_owner.h"

namespace sbk::engine
{
    class game_object;
    class bus;

    /**
     * @brief Audio runtime, powered by Sound Chef.
     * 
     * Owns the listener and master bus.
     * 
     * The master bus is currently acting as a "sink". All audio eventually goes to it.
     * The editor is free to create its own master busses, but the audio will still route through this master bus eventually.
     * 
     * The intention is that the runtime never cares about the editor and the editor just becomes a database of objects.
     */
    class runtime final : public sc_system, public sbk::core::object_owner, public boost::noncopyable
	{
    public:
        runtime();
        ~runtime();

        auto init(const sc_system_config& config) -> sbk::result<>;

        [[nodiscard]] auto get_listener_game_object() const -> std::shared_ptr<sbk::engine::game_object>;
        [[nodiscard]] auto get_master_bus() const -> std::shared_ptr<sbk::engine::bus>;

    private:
        bool m_initSoundChef = false;

        std::weak_ptr<sbk::engine::game_object> m_listenerGameObject;
        std::weak_ptr<sbk::engine::bus> m_masterBus;
	};
}