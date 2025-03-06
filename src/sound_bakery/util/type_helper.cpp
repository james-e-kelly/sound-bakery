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

using namespace sbk::util;

SB_OBJECT_CATEGORY type_helper::getCategoryFromType(rttr::type type)
{
    SB_OBJECT_CATEGORY category = SB_CATEGORY_UNKNOWN;

    if (!type.is_valid())
    {
        return category;
    }

    if (type.is_derived_from(rttr::type::get<sbk::engine::bus>()))
    {
        category = SB_CATEGORY_BUS;
    }
    else if (type.is_derived_from(rttr::type::get<sbk::engine::container>()))
    {
        category = SB_CATEGORY_NODE;
    }
    else if (type == rttr::type::get<sbk::engine::event>())
    {
        category = SB_CATEGORY_EVENT;
    }
    else if (type == rttr::type::get<sbk::engine::sound>())
    {
        category = SB_CATEGORY_SOUND;
    }
    else if (type == rttr::type::get<sbk::engine::float_parameter>() ||
             type == rttr::type::get<sbk::engine::int_parameter>() ||
             type == rttr::type::get<sbk::engine::named_parameter>())
    {
        category = SB_CATEGORY_PARAMETER;
    }
    else if (type == rttr::type::get<sbk::engine::soundbank>())
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
        BOOST_ASSERT(false && "Could not get category for type");
    }

    return category;
}

std::unordered_set<rttr::type> type_helper::getTypesFromCategory(SB_OBJECT_CATEGORY category)
{
    std::unordered_set<rttr::type> result;

    switch (category)
    {
        case SB_CATEGORY_NODE:
            result.insert(rttr::type::get<sbk::engine::BlendContainer>());
            result.insert(rttr::type::get<sbk::engine::RandomContainer>());
            result.insert(rttr::type::get<sbk::engine::sequence_container>());
            result.insert(rttr::type::get<sbk::engine::sound_container>());
            result.insert(rttr::type::get<sbk::engine::switch_container>());
            break;
        case SB_CATEGORY_BUS:
            result.insert(rttr::type::get<sbk::engine::bus>());
            result.insert(rttr::type::get<sbk::engine::aux_bus>());
            break;
        case SB_CATEGORY_MUSIC:
            break;
        case SB_CATEGORY_EVENT:
            result.insert(rttr::type::get<sbk::engine::event>());
            break;
        case SB_CATEGORY_BANK:
            result.insert(rttr::type::get<sbk::engine::soundbank>());
            break;
        case SB_CATEGORY_SOUND:
            result.insert(rttr::type::get<sbk::engine::sound>());
            break;
        case SB_CATEGORY_PARAMETER:
            result.insert(rttr::type::get<sbk::engine::float_parameter>());
            result.insert(rttr::type::get<sbk::engine::named_parameter>());
            break;
        case SB_CATEGORY_NUM:
            break;
        default:
            break;
    }

    return result;
}

rttr::string_view type_helper::get_display_name_from_type(rttr::type type)
{
    rttr::string_view result = type.get_name();

    if (type == rttr::type::get<sbk::engine::sound_container>())
    {
        result = "Sound";
    }
    else if (type == rttr::type::get<sbk::engine::RandomContainer>())
    {
        result = "Random";
    }
    else if (type == rttr::type::get<sbk::engine::switch_container>())
    {
        result = "Switch";
    }
    else if (type == rttr::type::get<sbk::engine::sequence_container>())
    {
        result = "Sequence";
    }
    else if (type == rttr::type::get<sbk::engine::BlendContainer>())
    {
        result = "Blend";
    }
    else if (type == rttr::type::get<sbk::engine::container>())
    {
        result = "Container";
    }
    else if (type == rttr::type::get<sbk::engine::bus>())
    {
        result = "Bus";
    }
    else if (type == rttr::type::get<sbk::engine::aux_bus>())
    {
        result = "Aux";
    }
    else if (type == rttr::type::get<sbk::engine::sound>())
    {
        result = "Sound";
    }
    else if (type == rttr::type::get<sbk::engine::event>())
    {
        result = "Event";
    }
    else if (type == rttr::type::get<sbk::engine::float_parameter>())
    {
        result = "Parameter";
    }
    else if (type == rttr::type::get<sbk::engine::named_parameter>())
    {
        result = "Switch";
    }
    else if (type == rttr::type::get<sbk::engine::named_parameter_value>())
    {
        result = "Switch Value";
    }
    else if (type == rttr::type::get<sbk::engine::soundbank>())
    {
        result = "Soundbank";
    }

    return result;
}

std::string sbk::util::type_helper::getFolderNameForObjectType(rttr::type type)
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

std::string_view type_helper::getFileExtensionOfObjectCategory(SB_OBJECT_CATEGORY category)
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

std::string_view type_helper::getPayloadFromType(rttr::type type)
{
    std::string_view result = "OBJECT";

    if (type == rttr::type::get<sbk::engine::sound>())
    {
        result = sbk::editor::PayloadSound;
    }
    else if (type.is_derived_from<sbk::engine::bus>())
    {
        result = sbk::editor::PayloadBus;
    }
    else if (type.is_derived_from<sbk::engine::container>())
    {
        result = sbk::editor::PayloadContainer;
    }
    else if (type == rttr::type::get<sbk::engine::float_parameter>())
    {
        result = sbk::editor::PayloadFloatParam;
    }
    else if (type == rttr::type::get<sbk::engine::named_parameter>())
    {
        result = sbk::editor::PayloadNamedParam;
    }
    else if (type == rttr::type::get<sbk::engine::named_parameter_value>())
    {
        result = sbk::editor::PayloadIntParamValue;
    }
    else if (type.is_derived_from<sbk::core::database_object>())
    {
        result = sbk::editor::PayloadObject;
    }

    return result;
}

bool type_helper::isTypePlayable(const rttr::type& type)
{
    bool result = false;

    if (type.is_valid())
    {
        result = type.is_derived_from<sbk::engine::container>() || type.is_derived_from<sbk::engine::sound>() ||
                 type.is_derived_from<sbk::engine::event>();
    }

    return result;
}

rttr::enumeration type_helper::getObjectCategoryEnum()
{
    return rttr::type::get<SB_OBJECT_CATEGORY>().get_enumeration();
}

rttr::string_view type_helper::getObjectCategoryName(const SB_OBJECT_CATEGORY& objectCategory)
{
    static const rttr::string_view defaultName  = "Unknown";
    const rttr::enumeration objectCategoryEnum  = getObjectCategoryEnum();

    rttr::string_view name = objectCategoryEnum.value_to_name(objectCategory);
    return name.data() ? name : defaultName;
}

sbk::core::object* type_helper::getObjectFromInstance(const rttr::instance& instance)
{
    return instance.try_convert<sbk::core::object>();
}

sbk::core::database_object* type_helper::getDatabaseObjectFromInstance(const rttr::instance& instance)
{
    return instance.try_convert<sbk::core::database_object>();
}

sbk::engine::node* type_helper::getNodeFromInstance(const rttr::instance& instance)
{
    return instance.try_convert<sbk::engine::node>();
}

sbk::engine::node_base* type_helper::getNodeBaseFromInstance(const rttr::instance& instance)
{
    return instance.try_convert<sbk::engine::node_base>();
}