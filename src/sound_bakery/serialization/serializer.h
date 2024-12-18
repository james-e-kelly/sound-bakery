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
            BOOST_ASSERT(type.is_class());
        }

        rttr::variant child;
        rttr::type type;

        friend class boost::serialization::access;

        template <class archive_class>
        void serialize(archive_class& archive, const unsigned int version)
        {
            for (rttr::property property : type.get_properties())
            {
                if (archive_class::is_loading())
                {
                    rttr::variant loaded;
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
            : originalVariant(variant), seqView(variant.create_sequential_view()), valueType(seqView.get_value_type())
        {
        }

        rttr::variant& originalVariant;
        rttr::variant_sequential_view seqView;
        rttr::type valueType;

        friend class boost::serialization::access;

        template <class archive_class>
        void serialize(archive_class& archive, const unsigned int version)
        {
            if (archive_class::is_loading())
            {
                size_t size = 0;
                archive& boost::serialization::make_nvp("Count", size);

                seqView.set_size(size);

                const bool needToCreate = size == 0;

                for (size_t index = 0; index < size; ++index)
                {
                    rttr::variant loadedVariant;
                    archive & boost::serialization::make_nvp("Item", loadedVariant);

                    if (needToCreate)
                    {
                        seqView.insert(seqView.begin() + index, loadedVariant);
                    }
                    else
                    {
                        seqView.set_value(index, loadedVariant);
                    }
                }
            }
            else
            {
                size_t size = seqView.get_size();
                archive & boost::serialization::make_nvp("Count", size);

                for (rttr::variant item : seqView)
                {
                    archive & boost::serialization::make_nvp("Item", item);
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

        template <class archive_class, typename variant_type, typename save_type>
        void serialize_variant_with_conversion(archive_class& archive, rttr::variant& variant)
        {
            if (archive_class::is_loading())
            {
                save_type loaded;
                archive & boost::serialization::make_nvp("Value", loaded);
                variant_type loadedConverted(loaded);
                variant = loadedConverted;
            }
            else
            {
                variant_type valueToSave = variant.convert<variant_type>();
                save_type valueToSaveConverted(valueToSave);
                archive & boost::serialization::make_nvp("Value", valueToSaveConverted);
            }
        }

        template <class archive_class>
        void serialize(archive_class& archive, rttr::variant& variant, const unsigned int version)
        {
            const rttr::type type = variant.get_type();

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
                serialize_variant_with_conversion<archive_class, std::string_view, std::string>(archive, variant);
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
            //     const rttr::variant_associative_view view = variant.create_associative_view();
            //     const rttr::type keyType                  = view.get_key_type();
            //     const rttr::type valueType                = view.is_key_only_type() ? view.get_key_type() : view.get_value_type();
            //
            //    if (!view.is_empty() && valueType.is_valid())
            //    {
            //        for (const rttr::variant& item : view)
            //        {
            //            const std::pair<rttr::variant, rttr::variant> valuePair =
            //                item.convert<std::pair<rttr::variant, rttr::variant>>();

            //            rttr::variant key   = valuePair.first.extract_wrapped_value();
            //            rttr::variant value = view.is_key_only_type() ? valuePair.first.extract_wrapped_value()
            //                                                          : valuePair.second.extract_wrapped_value();

            //            if (keyType.is_wrapper())
            //            {
            //                key.convert(keyType.get_wrapped_type());
            //            }

            //            if (valueType.is_wrapper())
            //            {
            //                value.convert(valueType.get_wrapped_type());
            //            }

            //            // For maps
            //            if (view.is_key_only_type() == false)
            //            {
            //                result &= saveVariant(emitter, rttr::string_view(), key);
            //            }

            //            result &= saveVariant(emitter, rttr::string_view(), value);
            //        }
            //    }
            }
            else if (type.is_sequential_container())
            {
                sbk::core::serialization::serialized_sequential_container serializedSequentialContainer(variant);
                archive& boost::serialization::make_nvp("SeqContainer", serializedSequentialContainer);
            }
            else if (type.is_class())
            {
                sbk::core::serialization::serialized_child_class childClass(variant);
                archive & boost::serialization::make_nvp("Child", childClass);

                if (archive_class::is_loading())
                {
                    variant = childClass.child;
                }
            }
        }

    }  // namespace serialization
}  // namespace boost