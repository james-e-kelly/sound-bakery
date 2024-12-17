#pragma once

#include <rttr/type.h>
#include <cstdint>

#include "sound_bakery/core/object/object_owner.h"

#include "yaml-cpp/yaml.h"

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
                archive & loadedValue;
                variant = loadedValue;
            }
            else
            {
                T valueToSave = variant.convert<T>();
                archive & valueToSave;
            }
        }

        template <class archive_class, typename variant_type, typename save_type>
        void serialize_variant_with_conversion(archive_class& archive, rttr::variant& variant)
        {
            if (archive_class::is_loading())
            {
                save_type loaded;
                archive & loaded;
                variant_type loadedConverted(loaded);
                variant = loadedConverted;
            }
            else
            {
                variant_type valueToSave = variant.convert<variant_type>();
                save_type valueToSaveConverted(valueToSave);
                archive & valueToSaveConverted;
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
                    archive & loadedStringValue;
                    variant = enumeration.name_to_value(loadedStringValue);
                }
                else
                {
                    const rttr::string_view enumValueName = enumeration.value_to_name(variant);

                    if (!enumValueName.empty())
                    {
                        std::string savingString = enumValueName.data();
                        archive & savingString;
                    }
                }
            }
            //else if (type.is_associative_container())
            //{
            //    const rttr::variant_associative_view view = variant.create_associative_view();
            //    const rttr::type keyType                  = view.get_key_type();
            //    const rttr::type valueType = view.is_key_only_type() ? view.get_key_type() : view.get_value_type();

            //    
            //    if (!view.is_empty() && valueType.is_valid())
            //    {
            //        ChildSequence childSeq(emitter, name);

            //        result = true;

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
            //}
            //else if (type.is_sequential_container())
            //{
            //    const rttr::variant_sequential_view view = variant.create_sequential_view();
            //    const rttr::type valueType               = view.get_value_type();

            //    ChildSequence childSeq(emitter, name);

            //    if (!view.is_empty() && valueType.is_valid())
            //    {
            //        for (rttr::variant item : view)
            //        {
            //            if (valueType.is_wrapper())
            //            {
            //                item.convert(valueType.get_wrapped_type());
            //            }

            //            item.convert(valueType);

            //            serialize(archive, item, version);
            //        }
            //    }
            //}
            //else if (type.is_class())
            //{
            //    for (rttr::property& property : type.get_properties())
            //    {
            //        rttr::variant propertyValue = property.get_value(variant);
            //        serialize(archive, propertyValue, version);
            //    }
            //}
            else
            {
                BOOST_ASSERT(false);
            }
        }

    }  // namespace serialization
}  // namespace boost

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
    struct SB_CLASS SaveData
    {
        rttr::string_view saveName;
        rttr::variant saveData;
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

    class SB_CLASS binary_serializer final : public serializer
    {
    public:
        auto save_object(const std::shared_ptr<sbk::core::object>& object, const std::filesystem::path& file) -> sb_result override;
        auto load_object(sbk::core::object_owner& objectOwner, const std::filesystem::path& file) -> sb_result override;
    };

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