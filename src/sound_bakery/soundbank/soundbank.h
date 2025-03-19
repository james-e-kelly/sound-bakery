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

    struct SB_CLASS soundbank_database_entry
    {
        std::string assetName;
        sbk_id assetID;

        template <class archive_class>
        void serialize(archive_class& archive, const unsigned int version)
        {
            archive & boost::serialization::make_nvp("AssetName", assetName);
            archive & boost::serialization::make_nvp("AssetID", assetID);
        }
    };

    /**
     * @brief Contains 
     */
    struct SB_CLASS soundbank_database
    {
        std::vector<soundbank_database_entry> database;

        auto fill_runtime_database() -> void;   //< Fill the sbk::engine::system database with the information serialized here
        
        template <class archive_class>
        void serialize(archive_class& archive, const unsigned int version)
        {
            archive & boost::serialization::make_nvp("LookupDatabase", database);
        }
    };

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

        soundbank_database lookupDatabase;
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

        auto set_init_soundbank(bool init) -> void { m_initSoundbank = init; }
        auto set_lookup_soundbank(bool lookup) -> void { m_lookupSoundbank = lookup; }

        auto is_init_soundbank() const -> bool { return m_initSoundbank; }
        auto is_lookup_soundbank() const -> bool { return m_lookupSoundbank; }

    private:
        std::vector<sbk::core::database_ptr<event>> m_events;
        bool m_initSoundbank = false; //< Determines whether we package bussess, parameters, etc.
        bool m_lookupSoundbank = false; //< Determines whether this bank contains string -> id lookup information
    };
}  // namespace sbk::engine