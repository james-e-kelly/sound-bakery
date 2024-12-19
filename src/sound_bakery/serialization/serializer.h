#pragma once

#include <rttr/type.h>
#include <cstdint>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/yaml_iarchive.hpp>
#include <boost/archive/yaml_oarchive.hpp>

#include "sound_bakery/core/object/object.h"
#include "sound_bakery/core/object/object_owner.h"

#include "yaml-cpp/yaml.h"

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
    static auto make_default_variant(const rttr::type& type) -> rttr::variant 
    { 
        BOOST_ASSERT(type.is_valid());

        if (type.is_wrapper())
        {
            BOOST_ASSERT_MSG(type.get_wrapped_type().is_arithmetic(),
                             "We only convert simple types to ensure we're not creating large objects accidentally");

            rttr::variant variant = 0u;
            variant.convert(type.get_wrapped_type());
            BOOST_ASSERT_MSG(variant.is_valid(), "Could not convert the default value to the wrapped type");
            variant.convert(type);
            BOOST_ASSERT_MSG(variant.is_valid(), "Could not convert the variant to the wrapping type");
            return variant;
        }
        else if (type.is_arithmetic()) // Can't construct arithmetic types
        {
            rttr::variant variant = 0u;
            variant.convert(type);
            BOOST_ASSERT_MSG(variant.is_valid(), "Could not convert the default value to type");
            return variant;
        }
        else if (type == rttr::type::get<std::string>())
        {
            BOOST_ASSERT(false);
            return rttr::variant(std::string());
        }
        else if (type == rttr::type::get<std::string_view>())
        {
            BOOST_ASSERT(false);
            return rttr::variant(std::string_view());
        }

        BOOST_ASSERT_MSG(type.get_constructor({}).is_valid(), "Types must have constructors at this point");
        return type.create_default();
    }

    struct SB_CLASS serialized_version
    {
        serialized_version() = default;

        unsigned int major = SBK_VERSION_MAJOR;
        unsigned int minor = SBK_VERSION_MINOR;
        unsigned int patch = SBK_VERSION_PATCH;

        auto valid() const -> bool
        { 
            return major || minor || patch;
        }

        auto version_compatible() const -> bool
        { 
            return major == SBK_VERSION_MAJOR && minor == SBK_VERSION_MINOR;
        }

        friend class boost::serialization::access;

        template <class archive_class>
        void serialize(archive_class& archive, const unsigned int version)
        {
            archive & boost::serialization::make_nvp("Major", major);
            archive & boost::serialization::make_nvp("Minor", minor);
            archive & boost::serialization::make_nvp("Patch", patch);
        }
    };

    struct SB_CLASS serialized_header
    {
        serialized_header() = default;
        serialized_header(const std::shared_ptr<sbk::core::object>& object)
            : type(object->get_object_type().get_name().data()), version()
        {
        }

        std::string type;
        serialized_version version;

        auto valid() const -> bool { return !type.empty() && version.valid(); }

        friend class boost::serialization::access;

        template <class archive_class>
        void serialize(archive_class& archive, const unsigned int fileVersion)
        {
            archive & boost::serialization::make_nvp("Type", type);
            archive & boost::serialization::make_nvp("Version", version);
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

        friend class boost::serialization::access;

        template <class archive_class>
        void serialize(archive_class& archive, const unsigned int version)
        {
            for (rttr::property property : type.get_properties())
            {
                if (archive_class::is_loading())
                {
                    rttr::variant loaded = make_default_variant(property.get_type());
                    if (!loaded.is_valid())
                    {
                        BOOST_ASSERT(loaded.is_valid());
                        loaded = make_default_variant(property.get_type());
                    }
                    archive & boost::serialization::make_nvp(property.get_name().data(), loaded);
                    property.set_value(child, loaded);
                }
                else
                {
                    rttr::variant variantToSave = property.get_value(child);
                    archive & boost::serialization::make_nvp(property.get_name().data(), variantToSave);
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

        friend class boost::serialization::access;

        template <class archive_class>
        void serialize(archive_class& archive, const unsigned int version)
        {
            if (archive_class::is_loading())
            {
                size_t size = 0;
                archive & boost::serialization::make_nvp("Count", size);

                view.set_size(size);

                const bool needToCreate = size == 0;

                for (size_t index = 0; index < size; ++index)
                {
                    rttr::variant loadedVariant = make_default_variant(valueType);
                    BOOST_ASSERT(loadedVariant.is_valid());
                    archive & boost::serialization::make_nvp("Item", loadedVariant);

                    if (needToCreate)
                    {
                        view.insert(view.begin() + index, loadedVariant);
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
                archive & boost::serialization::make_nvp("Count", size);

                for (rttr::variant item : view)
                {
                    archive & boost::serialization::make_nvp("Item", item);
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
        { }

        rttr::variant& originalVariant;
        rttr::variant_associative_view view;
        rttr::type valueType;
        rttr::type keyType;

        friend class boost::serialization::access;

        template <class archive_class>
        void serialize(archive_class& archive, const unsigned int version)
        {
            if (archive_class::is_loading())
            {
                size_t size = 0;
                archive& boost::serialization::make_nvp("Count", size);

                view.clear();

                for (std::size_t index = 0; index < size; ++index)
                {
                    if (view.is_key_only_type())
                    {
                        rttr::variant loadedKey = make_default_variant(keyType);
                        archive & boost::serialization::make_nvp("Key", loadedKey);
                        view.insert(loadedKey);
                    }
                    else
                    {
                        std::pair<rttr::variant, rttr::variant> loadedPair(make_default_variant(keyType), make_default_variant(valueType));
                        archive & boost::serialization::make_nvp("KeyValue", loadedPair);
                        view.insert(loadedPair.first, loadedPair.second);
                    }
                }
            }
            else
            {
                size_t size = view.get_size();
                archive & boost::serialization::make_nvp("Count", size);

                for (rttr::variant item : view)
                {
                    std::pair<rttr::variant, rttr::variant> valuePair =
                        item.convert<std::pair<rttr::variant, rttr::variant>>();

                    if (view.is_key_only_type())
                    {
                        rttr::variant key   = valuePair.first.extract_wrapped_value();
                        archive & boost::serialization::make_nvp("Key", key);
                    }
                    else
                    {
                        archive& boost::serialization::make_nvp("KeyValue", valuePair);
                    }
                }
            }
        }
    };

    /**
     * @brief Abstract class that handles serializing object to and from the disk.
     */
    class SB_CLASS serializer
    {
    public:
        virtual auto save_object(const std::shared_ptr<sbk::core::object>& object, const std::filesystem::path& file) -> sb_result = 0;
        virtual auto load_object(sbk::core::object_owner& objectOwner, const std::filesystem::path& file) -> sb_result = 0;
    };

    template <class load_archive, class save_archive>
    class SB_CLASS boost_serializer : public serializer
    {
    public:
        auto save_object(const std::shared_ptr<sbk::core::object>& object, const std::filesystem::path& file) -> sb_result override
        {
            SC_CHECK_ARG(object);
            SC_CHECK(!file.empty(), MA_INVALID_FILE);

            std::ofstream outputStream(file);
            save_archive archive(outputStream);
            serialized_header header(object);

            archive << boost::serialization::make_nvp("Header", header);
            archive << boost::serialization::make_nvp("Object", *object.get());
            return MA_SUCCESS;
        }

        auto load_object(sbk::core::object_owner& objectOwner, const std::filesystem::path& file) -> sb_result override
        {
            SC_CHECK(std::filesystem::exists(file), MA_INVALID_FILE);

            std::ifstream outputStream(file);
            load_archive archive(outputStream);

            serialized_header header;
            archive >> boost::serialization::make_nvp("Header", header);
            SC_CHECK(header.valid(), MA_INVALID_DATA);

            const rttr::type objectType = rttr::type::get_by_name(header.type);
            SC_CHECK(objectType.is_valid(), MA_INVALID_DATA);

            if (objectType.is_derived_from<sbk::core::database_object>())
            {
                std::shared_ptr<sbk::core::database_object> createdDatabaseObject =
                    objectOwner.create_database_object(objectType);
                SC_CHECK(createdDatabaseObject, MA_ERROR);

                archive >> boost::serialization::make_nvp("Object", *createdDatabaseObject.get());
            }
            else
            {
                std::shared_ptr<sbk::core::object> runtimeObject = objectOwner.create_runtime_object(objectType);
                SC_CHECK(runtimeObject, MA_ERROR);

                archive >> boost::serialization::make_nvp("Object", *runtimeObject.get());
            }

            return MA_SUCCESS;
        }
    };

    using binary_serializer = boost_serializer<boost::archive::binary_iarchive, boost::archive::binary_oarchive>;
    using text_serializer = boost_serializer<boost::archive::text_iarchive, boost::archive::text_oarchive>;
    using xml_serializer = boost_serializer<boost::archive::xml_iarchive, boost::archive::xml_oarchive>;
    using yaml_new_serializer = boost_serializer<boost::archive::yaml_iarchive, boost::archive::yaml_oarchive>;

    class SB_CLASS yaml_serializer final : public serializer
    {
    public:
        static void saveObject(sbk::core::object* object, YAML::Emitter& emitter);
        static void saveSystem(sbk::engine::system* system, YAML::Emitter& emitter);

        static void packageSoundbank(sbk::engine::soundbank* soundbank, YAML::Emitter& emitter);
        static rttr::instance unpackSoundbank(YAML::Node& node);

        static void loadSystem(sbk::engine::system* system, YAML::Node& node);

        static rttr::instance createAndLoadObject(
            YAML::Node& node, std::optional<rttr::method> onLoadedMethod = std::optional<rttr::method>());

        static bool loadProperties(YAML::Node& node,
                                   rttr::instance instance,
                                   std::optional<rttr::method> onLoadedMethod = std::optional<rttr::method>());

    private:
        static bool saveInstance(YAML::Emitter& emitter, rttr::instance instance);
        static bool saveVariant(YAML::Emitter& emitter, rttr::string_view name, rttr::variant& variant);
        static bool saveStringVariant(YAML::Emitter& emitter, rttr::string_view name, rttr::variant variant);
        static bool saveEnumVariant(YAML::Emitter& emitter, rttr::string_view name, rttr::variant variant);
        static bool saveAssociateContainerVariant(YAML::Emitter& emitter,
                                                  rttr::string_view name,
                                                  rttr::variant variant);
        static bool saveSequentialContainerVariant(YAML::Emitter& emitter,
                                                   rttr::string_view name,
                                                   rttr::variant variant);
        static bool saveClassVariant(YAML::Emitter& emitter, rttr::string_view name, rttr::variant variant);
    };
}  // namespace sbk::core::serialization

namespace boost
{
    namespace serialization
    {
        template <class archive_class, typename T>
        void serialize_variant(archive_class& archive, rttr::variant& variant)
        {
            if (archive_class::is_loading())
            {
                T loadedValue;
                archive & boost::serialization::make_nvp("Value", loadedValue);
                variant = loadedValue;
            }
            else
            {
                T valueToSave = variant.convert<T>();
                archive & boost::serialization::make_nvp("Value", valueToSave);
            }
        }

        template <class archive_class>
        void serialize_variant_view(archive_class& archive, rttr::variant& variant)
        {
            if (archive_class::is_loading())
            {
                std::string loaded;
                archive & boost::serialization::make_nvp("Value", loaded);
                variant = loaded;
            }
            else
            {
                std::string_view valueToSave = variant.convert<std::string_view>();
                std::string valueToSaveConverted(valueToSave);
                archive & boost::serialization::make_nvp("Value", valueToSaveConverted);
            }
        }

        template <class archive_class>
        void serialize(archive_class& archive, rttr::variant& variant, const unsigned int version)
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
                serialize_variant_view<archive_class>(archive, variant);
            }
            else if (type.is_wrapper())
            {
                variant = variant.extract_wrapped_value();
                serialize(archive, variant, version);
            }
            else if (type.is_enumeration())
            {
                const rttr::enumeration enumeration = type.get_enumeration();

                if (archive_class::is_loading())
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
                archive & boost::serialization::make_nvp("SeqContainer", serializedSequentialContainer);
            }
            else if (type.is_class())
            {
                sbk::core::serialization::serialized_child_class childClass(variant);
                archive & boost::serialization::make_nvp("Child", childClass);
            }
        }

    }  // namespace serialization
}  // namespace boost