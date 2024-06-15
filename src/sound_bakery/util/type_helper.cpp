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
#include "sound_bakery/soundbank/soundbank.h"

using namespace sbk::Util;

SB_OBJECT_CATEGORY TypeHelper::getCategoryFromType(rttr::type type)
{
    SB_OBJECT_CATEGORY category = SB_CATEGORY_UNKNOWN;

    if (!type.is_valid())
    {
        return category;
    }

    if (type.is_derived_from(rttr::type::get<sbk::engine::Bus>()))
    {
        category = SB_CATEGORY_BUS;
    }
    else if (type.is_derived_from(rttr::type::get<sbk::engine::Container>()))
    {
        category = SB_CATEGORY_NODE;
    }
    else if (type == rttr::type::get<sbk::engine::Event>())
    {
        category = SB_CATEGORY_EVENT;
    }
    else if (type == rttr::type::get<sbk::engine::Sound>())
    {
        category = SB_CATEGORY_SOUND;
    }
    else if (type == rttr::type::get<sbk::engine::FloatParameter>() ||
             type == rttr::type::get<sbk::engine::NamedParameter>())
    {
        category = SB_CATEGORY_PARAMETER;
    }
    else if (type == rttr::type::get<sbk::engine::Soundbank>())
    {
        category = SB_CATEGORY_BANK;
    }
    else if (type.is_derived_from<sbk::core::database_object>())
    {
        category = SB_CATEGORY_DATABASE_OBJECT;
    }
    else if (type.is_derived_from<sbk::core::object>())
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
            result.insert(rttr::type::get<sbk::engine::BlendContainer>());
            result.insert(rttr::type::get<sbk::engine::RandomContainer>());
            result.insert(rttr::type::get<sbk::engine::SequenceContainer>());
            result.insert(rttr::type::get<sbk::engine::SoundContainer>());
            result.insert(rttr::type::get<sbk::engine::SwitchContainer>());
            break;
        case SB_CATEGORY_BUS:
            result.insert(rttr::type::get<sbk::engine::Bus>());
            result.insert(rttr::type::get<sbk::engine::AuxBus>());
            break;
        case SB_CATEGORY_MUSIC:
            break;
        case SB_CATEGORY_EVENT:
            result.insert(rttr::type::get<sbk::engine::Event>());
            break;
        case SB_CATEGORY_BANK:
            result.insert(rttr::type::get<sbk::engine::Soundbank>());
            break;
        case SB_CATEGORY_SOUND:
            result.insert(rttr::type::get<sbk::engine::Sound>());
            break;
        case SB_CATEGORY_PARAMETER:
            result.insert(rttr::type::get<sbk::engine::FloatParameter>());
            result.insert(rttr::type::get<sbk::engine::NamedParameter>());
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

    if (type == rttr::type::get<sbk::engine::SoundContainer>())
    {
        result = "Sound";
    }
    else if (type == rttr::type::get<sbk::engine::RandomContainer>())
    {
        result = "Random";
    }
    else if (type == rttr::type::get<sbk::engine::SwitchContainer>())
    {
        result = "Switch";
    }
    else if (type == rttr::type::get<sbk::engine::SequenceContainer>())
    {
        result = "Sequence";
    }
    else if (type == rttr::type::get<sbk::engine::BlendContainer>())
    {
        result = "Blend";
    }
    else if (type == rttr::type::get<sbk::engine::Container>())
    {
        result = "Container";
    }
    else if (type == rttr::type::get<sbk::engine::Bus>())
    {
        result = "Bus";
    }
    else if (type == rttr::type::get<sbk::engine::AuxBus>())
    {
        result = "Aux";
    }
    else if (type == rttr::type::get<sbk::engine::Sound>())
    {
        result = "Sound";
    }
    else if (type == rttr::type::get<sbk::engine::Event>())
    {
        result = "Event";
    }
    else if (type == rttr::type::get<sbk::engine::FloatParameter>())
    {
        result = "Parameter";
    }
    else if (type == rttr::type::get<sbk::engine::NamedParameter>())
    {
        result = "Switch";
    }
    else if (type == rttr::type::get<sbk::engine::NamedParameterValue>())
    {
        result = "Switch Value";
    }
    else if (type == rttr::type::get<sbk::engine::Soundbank>())
    {
        result = "Soundbank";
    }

    return result;
}

std::string sbk::Util::TypeHelper::getFolderNameForObjectType(rttr::type type)
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
        case SB_CATEGORY_BANK:
            result = ".bank";
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

    if (type == rttr::type::get<sbk::engine::Sound>())
    {
        result = sbk::editor::PayloadSound;
    }
    else if (type.is_derived_from<sbk::engine::Bus>())
    {
        result = sbk::editor::PayloadBus;
    }
    else if (type.is_derived_from<sbk::engine::Container>())
    {
        result = sbk::editor::PayloadContainer;
    }
    else if (type == rttr::type::get<sbk::engine::FloatParameter>())
    {
        result = sbk::editor::PayloadFloatParam;
    }
    else if (type == rttr::type::get<sbk::engine::NamedParameter>())
    {
        result = sbk::editor::PayloadNamedParam;
    }
    else if (type == rttr::type::get<sbk::engine::NamedParameterValue>())
    {
        result = sbk::editor::PayloadIntParamValue;
    }
    else if (type.is_derived_from<sbk::core::database_object>())
    {
        result = sbk::editor::PayloadObject;
    }

    return result;
}

bool TypeHelper::isTypePlayable(const rttr::type& type)
{
    bool result = false;

    if (type.is_valid())
    {
        result = type.is_derived_from<sbk::engine::Container>() || type.is_derived_from<sbk::engine::Sound>() ||
                 type.is_derived_from<sbk::engine::Event>();
    }

    return result;
}

rttr::enumeration TypeHelper::getObjectCategoryEnum()
{
    return rttr::type::get<SB_OBJECT_CATEGORY>().get_enumeration();
}

rttr::string_view TypeHelper::getObjectCategoryName(const SB_OBJECT_CATEGORY& objectCategory)
{
    static const rttr::enumeration objectCategoryEnum = getObjectCategoryEnum();

    return objectCategoryEnum.value_to_name(objectCategory);
}

sbk::core::object* TypeHelper::getObjectFromInstance(const rttr::instance& instance)
{
    return instance.try_convert<sbk::core::object>();
}

sbk::core::database_object* TypeHelper::getDatabaseObjectFromInstance(const rttr::instance& instance)
{
    return instance.try_convert<sbk::core::database_object>();
}

sbk::engine::Node* TypeHelper::getNodeFromInstance(const rttr::instance& instance)
{
    return instance.try_convert<sbk::engine::Node>();
}

sbk::engine::NodeBase* TypeHelper::getNodeBaseFromInstance(const rttr::instance& instance)
{
    return instance.try_convert<sbk::engine::NodeBase>();
}