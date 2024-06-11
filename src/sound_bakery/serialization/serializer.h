#pragma once

#include <rttr/type.h>

#include "yaml-cpp/yaml.h"

namespace SB::Core
{
    class object;
}

namespace SB::Engine
{
    class Soundbank;
    class System;
}  // namespace SB::Engine

namespace SB::Core::Serialization
{
    struct SB_CLASS SaveData
    {
        rttr::string_view saveName;
        rttr::variant saveData;
    };

    class SB_CLASS Serializer final
    {
    public:
        static void saveObject(SB::Core::object* object, YAML::Emitter& emitter);
        static void saveSystem(SB::Engine::System* system, YAML::Emitter& emitter);

        static void packageSoundbank(SB::Engine::Soundbank* soundbank, YAML::Emitter& emitter);
        static rttr::instance unpackSoundbank(YAML::Node& node);

        static void loadSystem(SB::Engine::System* system, YAML::Node& node);

        static rttr::instance createAndLoadObject(
            YAML::Node& node, std::optional<rttr::method> onLoadedMethod = std::optional<rttr::method>());

    private:
        static bool loadProperties(YAML::Node& node,
                                   rttr::instance instance,
                                   std::optional<rttr::method> onLoadedMethod = std::optional<rttr::method>());

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
}  // namespace SB::Core::Serialization
