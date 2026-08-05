#pragma once

#include "sound_bakery/core/database/database_object.h"
#include "sound_bakery/core/object/object_owner.h"
#include "sound_bakery/core/error/result.h"
#include "sound_bakery/event/event.h"
#include "sound_bakery/node/bus/bus.h"
#include "sound_bakery/serialization/eastl_serialization.h"
#include "sound_bakery/sound/sound.h"
#include "sound_bakery/soundbank/soundbank.h"
#include "sound_bakery/system.h"

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/yaml_iarchive.hpp>
#include <boost/archive/yaml_oarchive.hpp>
#include <boost/serialization/binary_object.hpp>
#include <boost/serialization/vector.hpp>
#include <rttr/type.h>

namespace sbk::core
{
    class object;
}

namespace sbk::engine
{
    class soundbank;
    class system;
}  // namespace sbk::engine

namespace sbk::core::serialization
{
    enum class sound_bakery_serialization_version : int
    {
        start             = 1,
        soundbanks_lookup = 2,  //< Soundbanks can contain lookup info for integrations to get a list of all objects
        new_type_names    = 3,

        /** ADD NEW VERSIONS ABOVE */
        plus_one,
        cur = plus_one - 1
    };

    auto make_default_variant(const rttr::type& type) -> rttr::variant;
    auto read_binary_file(const std::filesystem::path& file) -> eastl::vector<uint8_t>;

    /**
     * @brief Stores the version of Sound Bakery.
     *
     * Once loaded, can check whether the serialized version is compatible with this version.
     */
    struct SB_CLASS serialized_version
    {
        unsigned int major = SBK_VERSION_MAJOR;
        unsigned int minor = SBK_VERSION_MINOR;
        unsigned int patch = SBK_VERSION_PATCH;

        [[nodiscard]] auto version_compatible() const -> bool { return SBK_VERSION_MAJOR == major && SBK_VERSION_MINOR >= minor; }

        template <class archive_class>
        auto serialize(archive_class& archive, const unsigned int version) -> void
        {
            archive& boost::serialization::make_nvp("Major", major);
            archive& boost::serialization::make_nvp("Minor", minor);
            archive& boost::serialization::make_nvp("Patch", patch);
        }
    };

    /**
     * @brief Serializes an object type so upon loading, we create the correct type.
     */
    struct SB_CLASS serialized_type
    {
        serialized_type() = default;
        serialized_type(const rttr::type& type) : typeString(type.get_name().data()) {}

        std::string typeString;

        [[nodiscard]] auto get_type() const -> rttr::type { return rttr::type::get_by_name(typeString); }

        template <class archive_class>
        auto serialize(archive_class& archive, const unsigned int fileVersion) -> void
        {
            archive& boost::serialization::make_nvp("Type", typeString);
        }
    };

    /**
     * @brief Serializes an object type and the object's data.
     */
    struct SB_CLASS serialized_object
    {
        serialized_object() = delete;
        serialized_object(const std::shared_ptr<sbk::core::database_object>& object, sbk::core::object_owner* objectOwner)
            : object(object), objectOwner(objectOwner)
        {
            if (object)
            {
                type.typeString = object->get_object_type().get_name().data();
                id              = object->get_database_id();
            }
        }

        operator sbk_id() const
        {
            return id;
        }

        serialized_type type;
        sbk_id id = 0;

        std::shared_ptr<sbk::core::database_object> object;
        sbk::core::object_owner* objectOwner = nullptr;

        template <class archive_class>
        auto serialize(archive_class& archive, const unsigned int v) -> void
        {
            archive& boost::serialization::make_nvp("Type", type);
            archive& boost::serialization::make_nvp("ID", id);

            if (typename archive_class::is_loading())
            {
                BOOST_ASSERT(objectOwner != nullptr);
                std::weak_ptr<sbk::core::database_object> foundObject = sbk::engine::system::get()->try_find_database_object(id);

                if (foundObject.expired())
                {
                    auto createResult = objectOwner->create_database_object(type.get_type(), false);
                    if (createResult.has_value())
                    {
                        object = createResult.value();
                        object->set_flags(object_flags::loading);
                    }
                }
                else
                {
                    object = foundObject.lock();
                    objectOwner->add_reference_to_object(object);
                }
            }

            BOOST_ASSERT(object);
            archive& boost::serialization::make_nvp("ObjectData", *object.get());

            if (typename archive_class::is_loading())
            {
                BOOST_ASSERT(sbk::engine::system::get() != nullptr);
                object->clear_flags(object_flags::loading);
                (void)sbk::engine::system::get()->add_object_to_database(object);
            }
        }
    };

    /**
     * @brief Header for an object that saves to a single file.
     *
     * Contains version information, the object type and its property data.
     */
    struct SB_CLASS serialized_standalone_object
    {
        serialized_standalone_object() = delete;
        serialized_standalone_object(const std::shared_ptr<sbk::core::database_object>& object, sbk::core::object_owner* objectOwner)
            : object(object, objectOwner) {}

        serialized_version version;
        serialized_object object;

        operator sbk_id() const { return object.id; }

        template <class archive_class>
        auto serialize(archive_class& archive, const unsigned int v) -> void
        {
            archive& boost::serialization::make_nvp("Version", version);
            BOOST_ASSERT_MSG(version.version_compatible(), "Cross version serialization not implemented yet");
            archive& boost::serialization::make_nvp("Object", object);
        }
    };

    struct SB_CLASS serialized_system
    {
        serialized_system() = delete;
        serialized_system(const std::shared_ptr<sbk::core::database_object>& object, sbk::core::object_owner* objectOwner) {}

        serialized_version version;

        operator sbk_id() const { return 0; }

        template <class archive_class>
        auto serialize(archive_class& archive, const unsigned int v) -> void
        {
            sbk::engine::system* system = sbk::engine::system::get();
            BOOST_ASSERT(system != nullptr);

            archive& boost::serialization::make_nvp("Version", version);
            BOOST_ASSERT_MSG(version.version_compatible(), "Cross version serialization not implemented yet");
            archive& boost::serialization::make_nvp("System", *system);
        }
    };

    template <class object_class>
    struct SB_CLASS serialized_object_vector
    {
        serialized_object_vector() = delete;
        serialized_object_vector(sbk::core::object_owner* objectOwner) : objectOwner(objectOwner) {}
        serialized_object_vector(const eastl::vector<std::shared_ptr<object_class>>& objects)
            : objects(objects), count(objects.size()), objectOwner(nullptr) {}

        sbk::core::object_owner* objectOwner = nullptr;
        std::size_t count                    = 0;
        eastl::vector<std::shared_ptr<object_class>> objects;

        template <class archive_class>
        auto serialize(archive_class& archive, const unsigned int v) -> void
        {
            archive& boost::serialization::make_nvp("Count", count);

            for (std::size_t index = 0; index < count; ++index)
            {
                if (typename archive_class::is_loading())
                {
                    serialized_object serializedObject({}, objectOwner);
                    archive& boost::serialization::make_nvp("Object", serializedObject);
                }
                else
                {
                    std::shared_ptr<sbk::core::database_object> convertedObject =
                        std::static_pointer_cast<sbk::core::database_object, object_class>(objects[index]);
                    serialized_object serializedObject(convertedObject, objectOwner);
                    archive& boost::serialization::make_nvp("Object", serializedObject);
                }
            }
        }
    };

    struct SB_CLASS serialized_sound
    {
        serialized_sound() = delete;
        serialized_sound(const std::shared_ptr<sbk::engine::sound>& sound) : sound(sound) {}
        serialized_sound(const std::shared_ptr<sbk::core::database_object>& databaseSound)
            : sound(std::static_pointer_cast<sbk::engine::sound, sbk::core::database_object>(databaseSound)) {}

        std::shared_ptr<sbk::engine::sound> sound;

        template <class archive_class>
        auto serialize(archive_class& archive, const unsigned int v) -> void
        {
            BOOST_ASSERT(sound);

            if (typename archive_class::is_saving())
            {
                const sbk::engine::encoding_sound encodingSound = sound->get_encoding_sound_data();
                const bool validFile                            = std::filesystem::exists(encodingSound.encodedSoundPath) &&
                                       std::filesystem::is_regular_file(encodingSound.encodedSoundPath);
                const eastl::vector<uint8_t> buffer = validFile ? read_binary_file(encodingSound.encodedSoundPath) : eastl::vector<uint8_t>();
                std::size_t size                  = buffer.size();

                archive& boost::serialization::make_nvp("Size", size);
                archive& boost::serialization::make_nvp("Data", boost::serialization::make_binary_object(buffer.data(), size));
            }
            else
            {
                std::size_t size = 0;
                archive& boost::serialization::make_nvp("Size", size);

                sbk::engine::raw_sound_ptr rawSound(std::malloc(size));
                archive& boost::serialization::make_nvp("Data", boost::serialization::make_binary_object(rawSound.get(), size));

                sound->set_raw_sound_data(rawSound, size);
            }
        }
    };

    template <>
    struct SB_CLASS serialized_object_vector<sbk::engine::sound>
    {
        serialized_object_vector() = delete;
        serialized_object_vector(sbk::core::object_owner* objectOwner) : objectOwner(objectOwner) {}
        serialized_object_vector(const eastl::vector<std::shared_ptr<sbk::engine::sound>>& objects)
            : objects(objects), count(objects.size()), objectOwner(nullptr)
        {
        }

        sbk::core::object_owner* objectOwner = nullptr;
        std::size_t count                    = 0;
        eastl::vector<std::shared_ptr<sbk::engine::sound>> objects;

        template <class archive_class>
        auto serialize(archive_class& archive, const unsigned int v) -> void
        {
            archive& boost::serialization::make_nvp("Count", count);

            for (std::size_t index = 0; index < count; ++index)
            {
                if (typename archive_class::is_loading())
                {
                    serialized_object serializedObject({}, objectOwner);
                    archive& boost::serialization::make_nvp("Object", serializedObject);

                    serialized_sound serializedSound(serializedObject.object);
                    archive& boost::serialization::make_nvp("RawSound", serializedSound);
                }
                else
                {
                    serialized_object serializedObject(objects[index], objectOwner);
                    archive& boost::serialization::make_nvp("Object", serializedObject);

                    serialized_sound serializedSound(objects[index]);
                    archive& boost::serialization::make_nvp("RawSound", serializedSound);
                }
            }
        }
    };

    struct SB_CLASS serialized_soundbank
    {
        serialized_soundbank() = delete;
        serialized_soundbank(const std::shared_ptr<sbk::core::database_object>& object, sbk::core::object_owner* objectOwner)
            : serializedSoundbank(object, objectOwner)
        {
        }

        serialized_version serializedVersion;
        serialized_object serializedSoundbank;

        operator sbk_id() const { return serializedSoundbank.id; }

        template <class archive_class>
        auto serialize(archive_class& archive, const unsigned int version) -> void
        {
            archive& boost::serialization::make_nvp("Version", serializedVersion);
            BOOST_ASSERT_MSG(serializedVersion.version_compatible(), "Cross version serialization not implemented yet");

            archive& boost::serialization::make_nvp("Soundbank", serializedSoundbank);
            BOOST_ASSERT_MSG(serializedSoundbank.object->get_object_type().is_derived_from<sbk::engine::soundbank>(), "Must be saving a soundbank type");

            sbk::engine::soundbank* soundbank = serializedSoundbank.object->try_convert_object<sbk::engine::soundbank>();

            if (typename archive_class::is_loading())
            {
                serialized_object_vector<sbk::engine::sound> serializedSounds(soundbank);
                serialized_object_vector<sbk::engine::node_base> serializedNodes(soundbank);
                serialized_object_vector<sbk::engine::event> serializedEvents(soundbank);

                archive& boost::serialization::make_nvp("Sounds", serializedSounds);
                archive& boost::serialization::make_nvp("Nodes", serializedNodes);
                archive& boost::serialization::make_nvp("Events", serializedEvents);

                if (soundbank->is_init_soundbank())
                {
                    serialized_object_vector<sbk::engine::bus> serializedBusses(soundbank);
                    serialized_object_vector<sbk::engine::int_parameter> serializedIntParameters(soundbank);
                    serialized_object_vector<sbk::engine::float_parameter> serializedFloatParameters(soundbank);
                    serialized_object_vector<sbk::engine::named_parameter> serializedNamedParameters(soundbank);

                    archive& boost::serialization::make_nvp("Busses", serializedBusses);
                    archive& boost::serialization::make_nvp("IntParameters", serializedIntParameters);
                    archive& boost::serialization::make_nvp("FloatParameters", serializedFloatParameters);
                    archive& boost::serialization::make_nvp("NamedParameters", serializedNamedParameters);
                }

                if (version >= static_cast<unsigned int>(sound_bakery_serialization_version::soundbanks_lookup))
                {
                    if (soundbank->is_lookup_soundbank())
                    {
                        sbk::engine::soundbank_database database;
                        archive& boost::serialization::make_nvp("Database", database);
                    }
                }
            }
            else
            {
                sbk::engine::soundbank_dependencies soundbankDependencies = soundbank->gather_dependencies();

                serialized_object_vector<sbk::engine::sound> serializedSounds(soundbankDependencies.sounds);
                serialized_object_vector<sbk::engine::node_base> serializedNodes(soundbankDependencies.nodes);
                serialized_object_vector<sbk::engine::event> serializedEvents(soundbankDependencies.events);

                archive& boost::serialization::make_nvp("Sounds", serializedSounds);
                archive& boost::serialization::make_nvp("Nodes", serializedNodes);
                archive& boost::serialization::make_nvp("Events", serializedEvents);

                if (soundbank->is_init_soundbank())
                {
                    serialized_object_vector<sbk::engine::bus> serializedBusses(soundbankDependencies.busses);
                    serialized_object_vector<sbk::engine::int_parameter> serializedIntParameters(soundbankDependencies.intParameters);
                    serialized_object_vector<sbk::engine::float_parameter> serializedFloatParameters(soundbankDependencies.floatParameters);
                    serialized_object_vector<sbk::engine::named_parameter> serializedNamedParameters(soundbankDependencies.namedParameters);

                    archive& boost::serialization::make_nvp("Busses", serializedBusses);
                    archive& boost::serialization::make_nvp("IntParameters", serializedIntParameters);
                    archive& boost::serialization::make_nvp("FloatParameters", serializedFloatParameters);
                    archive& boost::serialization::make_nvp("NamedParameters", serializedNamedParameters);
                }

                if (soundbank->is_lookup_soundbank())
                {
                    archive& boost::serialization::make_nvp("Database", soundbankDependencies.lookupDatabase);
                }
            }
        }
    };

    struct SB_CLASS serialized_child_class
    {
        serialized_child_class() = default;
        serialized_child_class(rttr::variant& variant) : child(variant), type(variant.get_type())
        {
            BOOST_ASSERT(variant.is_valid());
            BOOST_ASSERT(type.is_class());
        }

        rttr::variant& child;
        rttr::type type;

        template <class archive_class>
        auto serialize(archive_class& archive, const unsigned int version) -> void
        {
            for (rttr::property property : type.get_properties())
            {
                BOOST_ASSERT(property.is_valid());
                BOOST_ASSERT(property.get_type().is_valid());

                if (typename archive_class::is_loading())
                {
                    rttr::variant loaded = make_default_variant(property.get_type());
                    BOOST_ASSERT(loaded.is_valid());
                    archive& boost::serialization::make_nvp(property.get_name().data(), loaded);
                    loaded.convert(property.get_type());
                    BOOST_ASSERT(loaded.get_type() == property.get_type());
                    property.set_value(child, loaded);
                }
                else
                {
                    rttr::variant variantToSave = property.get_value(child);
                    archive& boost::serialization::make_nvp(property.get_name().data(), variantToSave);
                }
            }
        }
    };

    struct SB_CLASS serialized_sequential_container
    {
        serialized_sequential_container() = delete;
        serialized_sequential_container(rttr::variant& variant)
            : originalVariant(variant), view(variant.create_sequential_view()), valueType(view.get_value_type())
        {
        }

        rttr::variant& originalVariant;
        rttr::variant_sequential_view view;
        rttr::type valueType;

        template <class archive_class>
        auto serialize(archive_class& archive, const unsigned int version) -> void
        {
            if (typename archive_class::is_loading())
            {
                size_t size = 0;
                archive& boost::serialization::make_nvp("Count", size);

                view.set_size(size);

                const bool needToCreate = size == 0;

                for (size_t index = 0; index < size; ++index)
                {
                    rttr::variant loadedVariant = make_default_variant(valueType);
                    BOOST_ASSERT(loadedVariant.is_valid());
                    archive& boost::serialization::make_nvp("Item", loadedVariant);

                    loadedVariant.convert((rttr::type)valueType);
                    if (needToCreate)
                    {
                        view.insert(view.begin() + static_cast<int>(index), loadedVariant);
                    }
                    else
                    {
                        view.set_value(index, loadedVariant);
                    }
                }
            }
            else
            {
                size_t size = view.get_size();
                archive& boost::serialization::make_nvp("Count", size);

                for (rttr::variant item : view)
                {
                    archive& boost::serialization::make_nvp("Item", item);
                }
            }
        }
    };

    struct SB_CLASS serialized_associative_container
    {
        serialized_associative_container() = delete;
        serialized_associative_container(rttr::variant& variant)
            : originalVariant(variant),
              view(variant.create_associative_view()),
              keyType(view.get_key_type()),
              valueType(view.is_key_only_type() ? view.get_key_type()
                                                : view.get_value_type())
        {
        }

        rttr::variant& originalVariant;
        rttr::variant_associative_view view;
        rttr::type valueType;
        rttr::type keyType;

        template <class archive_class>
        auto serialize(archive_class& archive, const unsigned int version) -> void
        {
            if (typename archive_class::is_loading())
            {
                size_t size = 0;
                archive& boost::serialization::make_nvp("Count", size);

                view.clear();

                for (std::size_t index = 0; index < size; ++index)
                {
                    if (view.is_key_only_type())
                    {
                        rttr::variant loadedKey = make_default_variant(keyType);
                        archive& boost::serialization::make_nvp("Key", loadedKey);
                        loadedKey.convert((rttr::type)keyType);
                        view.insert(loadedKey);
                    }
                    else
                    {
                        std::pair<rttr::variant, rttr::variant> loadedPair(make_default_variant(keyType), make_default_variant(valueType));
                        archive& boost::serialization::make_nvp("KeyValue", loadedPair);
                        loadedPair.first.convert((rttr::type)keyType);
                        loadedPair.second.convert((rttr::type)valueType);
                        view.insert(loadedPair.first, loadedPair.second);
                    }
                }
            }
            else
            {
                size_t size = view.get_size();
                archive& boost::serialization::make_nvp("Count", size);

                for (rttr::variant item : view)
                {
                    std::pair<rttr::variant, rttr::variant> valuePair =
                        item.convert<std::pair<rttr::variant, rttr::variant>>();

                    if (view.is_key_only_type())
                    {
                        rttr::variant key = valuePair.first.extract_wrapped_value();
                        archive& boost::serialization::make_nvp("Key", key);
                    }
                    else
                    {
                        archive& boost::serialization::make_nvp("KeyValue", valuePair);
                    }
                }
            }
        }
    };

    template <class load_archive,
              class save_archive,
              std::ios_base::openmode inputMode,
              std::ios_base::openmode outputMode>
    class SB_CLASS boost_serializer
    {
    public:
        template <class serialize_class>
        auto save_database_object(std::shared_ptr<sbk::core::database_object>& object, const std::filesystem::path& file) -> sbk::result<void>
        {
            SBK_CHECK(object, SBK_ERR_INVALID_PARAMETER);
            SBK_CHECK(!file.empty(), SBK_ERR_INVALID_FILE);

            try
            {
                std::ofstream outputStream(file, outputMode);
                save_archive archive(outputStream);
                serialize_class serialize(object, nullptr);

                archive& boost::serialization::make_nvp("Data", serialize);
            }
            catch (const std::exception& exception)
            {
                return sbk::make_error(SBK_ERR_BAKERY_SERIALIZATION, fmt::format("{} exception thrown when saving '{}' [{}] to {}", exception.what(), static_cast<const char*>(object->get_database_name()), object->get_database_id(), file.string()));
            }

            return sbk::ok();
        }

        auto save_system(const std::filesystem::path& file) -> sbk::result<void>
        {
            SBK_CHECK(!file.empty(), SBK_ERR_INVALID_FILE);

            try
            {
                std::ofstream outputStream(file, outputMode);
                save_archive archive(outputStream);
                serialized_system serialize({}, nullptr);

                archive& boost::serialization::make_nvp("System", serialize);
            }
            catch (const std::exception& exception)
            {
                return sbk::make_error(SBK_ERR_BAKERY_SERIALIZATION, fmt::format("{} exception thrown when saving the system to {}", exception.what(), file.string()));
            }

            return sbk::ok();
        }

        template <class serialize_class>
        auto load_object(sbk::core::object_owner* objectOwner, const std::filesystem::path& file) -> sbk::result<sbk_id>
        {
            SBK_CHECK(std::filesystem::exists(file), SBK_ERR_INVALID_FILE);

            try
            {
                std::ifstream inputStream(file, inputMode);
                load_archive archive(inputStream);
                serialize_class object({}, objectOwner);

                archive& boost::serialization::make_nvp("Data", object);
                return static_cast<sbk_id>(object);
            }
            catch (const std::exception& exception)
            {
                return sbk::make_error(SBK_ERR_BAKERY_SERIALIZATION, fmt::format("{} exception thrown when loading {}", exception.what(), file.string()));
            }
        }
    };

    using binary_serializer = boost_serializer<boost::archive::binary_iarchive, boost::archive::binary_oarchive, std::ios_base::in | std::ios_base::binary, std::ios_base::out | std::ios_base::binary>;
    using text_serializer   = boost_serializer<boost::archive::text_iarchive, boost::archive::text_oarchive, std::ios_base::in, std::ios_base::out>;
    using xml_serializer    = boost_serializer<boost::archive::xml_iarchive, boost::archive::xml_oarchive, std::ios_base::in, std::ios_base::out>;
    using yaml_serializer   = boost_serializer<boost::archive::yaml_iarchive, boost::archive::yaml_oarchive, std::ios_base::in, std::ios_base::out>;
}  // namespace sbk::core::serialization

BOOST_CLASS_VERSION(sbk::core::serialization::serialized_type, static_cast<int>(sbk::core::serialization::sound_bakery_serialization_version::cur))
BOOST_CLASS_VERSION(sbk::core::serialization::serialized_object, static_cast<int>(sbk::core::serialization::sound_bakery_serialization_version::cur))
BOOST_CLASS_VERSION(sbk::core::serialization::serialized_standalone_object, static_cast<int>(sbk::core::serialization::sound_bakery_serialization_version::cur))
BOOST_CLASS_VERSION(sbk::core::serialization::serialized_system, static_cast<int>(sbk::core::serialization::sound_bakery_serialization_version::cur))
BOOST_CLASS_VERSION(sbk::core::serialization::serialized_sound, static_cast<int>(sbk::core::serialization::sound_bakery_serialization_version::cur))
BOOST_CLASS_VERSION(sbk::core::serialization::serialized_soundbank, static_cast<int>(sbk::core::serialization::sound_bakery_serialization_version::cur))
BOOST_CLASS_VERSION(sbk::core::serialization::serialized_child_class, static_cast<int>(sbk::core::serialization::sound_bakery_serialization_version::cur))
BOOST_CLASS_VERSION(sbk::core::serialization::serialized_sequential_container, static_cast<int>(sbk::core::serialization::sound_bakery_serialization_version::cur))
BOOST_CLASS_VERSION(sbk::core::serialization::serialized_associative_container, static_cast<int>(sbk::core::serialization::sound_bakery_serialization_version::cur))

namespace boost
{
    namespace serialization
    {
        template <class archive_class, typename T>
        auto serialize_variant(archive_class& archive, rttr::variant& variant) -> void
        {
            if (typename archive_class::is_loading())
            {
                T loadedValue;
                archive& boost::serialization::make_nvp("Value", loadedValue);
                variant = loadedValue;
            }
            else
            {
                T valueToSave = variant.convert<T>();
                archive& boost::serialization::make_nvp("Value", valueToSave);
            }
        }

        template <class archive_class>
        auto serialize_variant_string_view(archive_class& archive, rttr::variant& variant) -> void
        {
            if (typename archive_class::is_loading())
            {
                std::string loaded;
                archive& boost::serialization::make_nvp("Value", loaded);
                variant = loaded;
            }
            else
            {
                std::string_view valueToSave = variant.convert<std::string_view>();
                std::string valueToSaveConverted(valueToSave);
                archive& boost::serialization::make_nvp("Value", valueToSaveConverted);
            }
        }

        template <class archive_class>
        auto serialize(archive_class& archive, rttr::variant& variant, const unsigned int version) -> void
        {
            const rttr::type type = variant.get_type();
            BOOST_ASSERT_MSG(type.is_valid(), "Type must be valid to load correctly");

            if (type.is_arithmetic())
            {
                if (type == rttr::type::get<bool>())
                {
                    serialize_variant<archive_class, bool>(archive, variant);
                }
                else if (type == rttr::type::get<int8_t>())
                {
                    serialize_variant<archive_class, int8_t>(archive, variant);
                }
                else if (type == rttr::type::get<int16_t>())
                {
                    serialize_variant<archive_class, int16_t>(archive, variant);
                }
                else if (type == rttr::type::get<int32_t>())
                {
                    serialize_variant<archive_class, int32_t>(archive, variant);
                }
                else if (type == rttr::type::get<int64_t>())
                {
                    serialize_variant<archive_class, int64_t>(archive, variant);
                }
                else if (type == rttr::type::get<uint8_t>())
                {
                    serialize_variant<archive_class, uint8_t>(archive, variant);
                }
                else if (type == rttr::type::get<uint16_t>())
                {
                    serialize_variant<archive_class, uint16_t>(archive, variant);
                }
                else if (type == rttr::type::get<uint32_t>())
                {
                    serialize_variant<archive_class, uint32_t>(archive, variant);
                }
                else if (type == rttr::type::get<uint64_t>())
                {
                    serialize_variant<archive_class, uint64_t>(archive, variant);
                }
                else if (type == rttr::type::get<float>())
                {
                    serialize_variant<archive_class, float>(archive, variant);
                }
                else if (type == rttr::type::get<double>())
                {
                    serialize_variant<archive_class, double>(archive, variant);
                }
            }
            else if (type == rttr::type::get<std::string>())
            {
                serialize_variant<archive_class, std::string>(archive, variant);
            }
            else if (type == rttr::type::get<std::string_view>())
            {
                serialize_variant_string_view<archive_class>(archive, variant);
            }
            else if (type.is_wrapper())
            {
                variant = variant.extract_wrapped_value();
                serialize(archive, variant, version);
            }
            else if (type.is_enumeration())
            {
                const rttr::enumeration enumeration = type.get_enumeration();

                if (typename archive_class::is_loading())
                {
                    std::string loadedStringValue;
                    archive& boost::serialization::make_nvp("Value", loadedStringValue);
                    variant = enumeration.name_to_value(loadedStringValue);
                }
                else
                {
                    const rttr::string_view enumValueName = enumeration.value_to_name(variant);

                    if (!enumValueName.empty())
                    {
                        std::string savingString = enumValueName.data();
                        archive& boost::serialization::make_nvp("Value", savingString);
                    }
                }
            }
            else if (type.is_associative_container())
            {
                sbk::core::serialization::serialized_associative_container serializedAssociativeContainer(variant);
                archive& boost::serialization::make_nvp("AssociativeContainer", serializedAssociativeContainer);
            }
            else if (type.is_sequential_container())
            {
                sbk::core::serialization::serialized_sequential_container serializedSequentialContainer(variant);
                archive& boost::serialization::make_nvp("SeqContainer", serializedSequentialContainer);
            }
            else if (type.is_class())
            {
                sbk::core::serialization::serialized_child_class childClass(variant);
                archive& boost::serialization::make_nvp("Child", childClass);
            }
        }

    }  // namespace serialization
}  // namespace boost