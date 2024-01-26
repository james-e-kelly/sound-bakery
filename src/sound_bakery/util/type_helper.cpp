#include "type_helper.h"

#include "sound_bakery/editor/editor_defines.h"
#include "sound_bakery/event/event.h"
#include "sound_bakery/node/bus/aux_bus.h"
#include "sound_bakery/node/bus/bus.h"
#include "sound_bakery/node/container/blend_container.h"
#include "sound_bakery/node/container/random_container.h"
#include "sound_bakery/node/container/sequence_container.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/node/container/switch_container.h"
#include "sound_bakery/parameter/parameter.h"
#include "sound_bakery/sound/sound.h"

using namespace SB::Util;

SB_OBJECT_CATEGORY TypeHelper::getCategoryFromType(rttr::type type)
{
    SB_OBJECT_CATEGORY category = SB_CATEGORY_UNKNOWN;

    if (type.is_derived_from(rttr::type::get<SB::Engine::Bus>()))
    {
        category = SB_CATEGORY_BUS;
    }
    else if (type.is_derived_from(rttr::type::get<SB::Engine::Container>()))
    {
        category = SB_CATEGORY_NODE;
    }
    else if (type == rttr::type::get<SB::Engine::Event>())
    {
        category = SB_CATEGORY_EVENT;
    }
    else if (type == rttr::type::get<SB::Engine::Sound>())
    {
        category = SB_CATEGORY_SOUND;
    }
    else if (type == rttr::type::get<SB::Engine::FloatParameter>() ||
             type == rttr::type::get<SB::Engine::IntParameter>())
    {
        category = SB_CATEGORY_PARAMETER;
    }
    else if (type.is_derived_from<SB::Core::DatabaseObject>())
    {
        category = SB_CATEGORY_DATABASE_OBJECT;
    }
    else if (type.is_derived_from<SB::Core::Object>())
    {
        category = SB_CATEGORY_RUNTIME_OBJECT;
    }
    else
    {
        assert(false && "Could not get category for type");
    }

    return category;
}

std::unordered_set<rttr::type> TypeHelper::getTypesFromCategory(SB_OBJECT_CATEGORY category)
{
    std::unordered_set<rttr::type> result;

    switch (category)
    {
        case SB_CATEGORY_NODE:
            result.insert(rttr::type::get<SB::Engine::BlendContainer>());
            result.insert(rttr::type::get<SB::Engine::RandomContainer>());
            result.insert(rttr::type::get<SB::Engine::SequenceContainer>());
            result.insert(rttr::type::get<SB::Engine::SoundContainer>());
            result.insert(rttr::type::get<SB::Engine::SwitchContainer>());
            break;
        case SB_CATEGORY_BUS:
            result.insert(rttr::type::get<SB::Engine::Bus>());
            result.insert(rttr::type::get<SB::Engine::AuxBus>());
            break;
        case SB_CATEGORY_MUSIC:
            break;
        case SB_CATEGORY_EVENT:
            result.insert(rttr::type::get<SB::Engine::Event>());
            break;
        case SB_CATEGORY_SOUND:
            result.insert(rttr::type::get<SB::Engine::Sound>());
            break;
        case SB_CATEGORY_PARAMETER:
            result.insert(rttr::type::get<SB::Engine::FloatParameter>());
            result.insert(rttr::type::get<SB::Engine::IntParameter>());
            break;
        case SB_CATEGORY_NUM:
            break;
        default:
            break;
    }

    return result;
}

std::string_view TypeHelper::getDisplayNameFromType(rttr::type type)
{
    std::string_view result = "Invalid. Cannot get display name for type. type_helper.cpp";

    if (type == rttr::type::get<SB::Engine::SoundContainer>())
    {
        result = "Sound";
    }
    else if (type == rttr::type::get<SB::Engine::RandomContainer>())
    {
        result = "Random";
    }
    else if (type == rttr::type::get<SB::Engine::SwitchContainer>())
    {
        result = "Switch";
    }
    else if (type == rttr::type::get<SB::Engine::SequenceContainer>())
    {
        result = "Sequence";
    }
    else if (type == rttr::type::get<SB::Engine::BlendContainer>())
    {
        result = "Blend";
    }
    else if (type == rttr::type::get<SB::Engine::Container>())
    {
        result = "Container";
    }
    else if (type == rttr::type::get<SB::Engine::Bus>())
    {
        result = "Bus";
    }
    else if (type == rttr::type::get<SB::Engine::AuxBus>())
    {
        result = "Aux";
    }
    else if (type == rttr::type::get<SB::Engine::Sound>())
    {
        result = "Sound";
    }
    else if (type == rttr::type::get<SB::Engine::Event>())
    {
        result = "Event";
    }
    else if (type == rttr::type::get<SB::Engine::FloatParameter>())
    {
        result = "Parameter";
    }
    else if (type == rttr::type::get<SB::Engine::IntParameter>())
    {
        result = "Switch";
    }
    else if (type == rttr::type::get<SB::Engine::IntParameterValue>())
    {
        result = "Switch Value";
    }

    return result;
}

std::string SB::Util::TypeHelper::getFolderNameForObjectType(rttr::type type)
{
    const rttr::string_view typeName = type.get_name();

    std::string typeNameString = typeName.to_string();

    const std::size_t lastColonCharacterPos = typeNameString.find_last_of(':') + 1;

    if (lastColonCharacterPos == std::string::npos || lastColonCharacterPos >= typeNameString.size())
    {
        return typeNameString;
    }

    return typeNameString.substr(lastColonCharacterPos, std::string::npos);
}

std::string_view TypeHelper::getFileExtensionOfObjectCategory(SB_OBJECT_CATEGORY category)
{
    std::string_view result = ".object";

    switch (category)
    {
        case SB_CATEGORY_NODE:
            result = ".node";
            break;
        case SB_CATEGORY_BUS:
            result = ".bus";
            break;
        case SB_CATEGORY_MUSIC:
            result = ".music";
            break;
        case SB_CATEGORY_EVENT:
            result = ".event";
            break;
        case SB_CATEGORY_SOUND:
            result = ".sound";
            break;
        case SB_CATEGORY_NUM:
            break;
        default:
            break;
    }

    return result;
}

std::string_view TypeHelper::getPayloadFromType(rttr::type type)
{
    std::string_view result = "OBJECT";

    if (type == rttr::type::get<SB::Engine::Sound>())
    {
        result = SB::Editor::PayloadSound;
    }
    else if (type.is_derived_from<SB::Engine::Bus>())
    {
        result = SB::Editor::PayloadBus;
    }
    else if (type.is_derived_from<SB::Engine::Container>())
    {
        result = SB::Editor::PayloadContainer;
    }
    else if (type == rttr::type::get<SB::Engine::FloatParameter>())
    {
        result = SB::Editor::PayloadFloatParam;
    }
    else if (type == rttr::type::get<SB::Engine::IntParameter>())
    {
        result = SB::Editor::PayloadIntParam;
    }
    else if (type == rttr::type::get<SB::Engine::IntParameterValue>())
    {
        result = SB::Editor::PayloadIntParamValue;
    }
    else if (type.is_derived_from<SB::Core::DatabaseObject>())
    {
        result = SB::Editor::PayloadObject;
    }

    return result;
}