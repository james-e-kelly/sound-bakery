#pragma once

#include "sound_bakery/core/core_include.h"
#include "sound_chef/sound_chef_encoder.h"

namespace sbk::engine
{
    class event;
    class sound;
    class node_base;

    /**
     * @brief Wraps all events, objects, and sounds needed to package a soundbank.
    */
    struct SB_CLASS soundbank_dependencies
    {
        std::vector<std::shared_ptr<sbk::engine::event>> events;
        std::vector<std::shared_ptr<sbk::engine::sound>> sounds;
        std::vector<std::shared_ptr<sbk::engine::node_base>> nodes;

        // The following properties are sent straight to Sound Chef
        // for soundbank generation.
        std::vector<const char*> encodedSoundPaths;
        std::vector<sc_encoding_format> encodingFormats;

        std::vector<std::string> encodedSoundPathsStrings;  //< used to keep sound paths alive while passing to Sound Chef
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

    private:
        std::vector<sbk::core::database_ptr<event>> m_events;
    };
}  // namespace sbk::engine