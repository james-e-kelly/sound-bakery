#pragma once

#include <rttr/type.h>

#include "sound_bakery/core/object/object_owner.h"

#include "yaml-cpp/yaml.h"

namespace boost
{
    namespace serialization
    {
        template <class archive_class>
        void serialize(archive_class& archive, rttr::variant& variant, const unsigned int version)
        {
            float f = variant.to_float();

            archive & f;
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