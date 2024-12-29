#pragma once

#include "sound_bakery/core/core_include.h"
#include "sound_chef/sound_chef_encoder.h"

namespace sbk::engine
{
    class bus;
    class event;
    class sound;
    class node_base;
    class int_parameter;
    class float_parameter;
    class named_parameter;

    /**
     * @brief Wraps all events, objects, and sounds needed to package a soundbank.
     */
    struct SB_CLASS soundbank_dependencies
    {
        std::vector<std::shared_ptr<sbk::engine::event>> events;
        std::vector<std::shared_ptr<sbk::engine::sound>> sounds;
        std::vector<std::shared_ptr<sbk::engine::node_base>> nodes;

        std::vector<std::shared_ptr<sbk::engine::bus>> busses;
        std::vector<std::shared_ptr<sbk::engine::int_parameter>> intParameters;
        std::vector<std::shared_ptr<sbk::engine::float_parameter>> floatParameters;
        std::vector<std::shared_ptr<sbk::engine::named_parameter>> namedParameters;
    };

    /**
     * @brief Packages events and dependent objects and sounds.
     */
    class SB_CLASS soundbank : public sbk::core::database_object
    {
        REGISTER_REFLECTION(soundbank, sbk::core::database_object)

    public:
        std::vector<sbk::core::database_ptr<event>> get_events() const { return m_events; }

        soundbank_dependencies gather_dependencies() const;

        auto set_master_soundbank(bool master) -> void { m_masterSoundbank = master; }
        auto get_master_soundbank() const -> bool { return m_masterSoundbank; }

    private:
        std::vector<sbk::core::database_ptr<event>> m_events;
        bool m_masterSoundbank = false; //< Determines whether we package bussess, parameters, etc.
    };
}  // namespace sbk::engine