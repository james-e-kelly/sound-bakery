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

auto type_comparator::operator()(const rttr::type lhs, const rttr::type rhs) const -> bool
{
    return std::strcmp(lhs.get_name().data(), rhs.get_name().data()) < 0;
}

auto type_helper::get_category_from_type(rttr::type type) -> SB_OBJECT_CATEGORY
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

auto type_helper::get_types_from_category(SB_OBJECT_CATEGORY category) -> std::set<rttr::type, type_comparator>
{
    std::set<rttr::type, type_comparator> result;

    switch (category)
    {
        case SB_CATEGORY_NODE:
            result.insert(rttr::type::get<sbk::engine::blend_container>());
            result.insert(rttr::type::get<sbk::engine::random_container>());
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

auto type_helper::get_display_name_from_type(rttr::type type) -> rttr::string_view
{
    rttr::string_view result = type.get_name();

    if (type == rttr::type::get<sbk::engine::sound_container>())
    {
        result = "Sound";
    }
    else if (type == rttr::type::get<sbk::engine::random_container>())
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
    else if (type == rttr::type::get<sbk::engine::blend_container>())
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

auto sbk::util::type_helper::get_folder_name_for_object_type(rttr::type type) -> std::string
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

auto type_helper::get_file_extension_of_object_category(SB_OBJECT_CATEGORY category) -> std::string_view
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

auto type_helper::get_payload_from_type(rttr::type type) -> std::string_view
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

auto type_helper::is_type_playable(const rttr::type& type) -> bool
{
    bool result = false;

    if (type.is_valid())
    {
        result = type.is_derived_from<sbk::engine::container>() || type.is_derived_from<sbk::engine::sound>() ||
                 type.is_derived_from<sbk::engine::event>();
    }

    return result;
}

auto type_helper::get_object_category_enum() -> rttr::enumeration
{
    return rttr::type::get<SB_OBJECT_CATEGORY>().get_enumeration();
}

auto type_helper::get_object_category_name(const SB_OBJECT_CATEGORY& objectCategory) -> rttr::string_view
{
    static const rttr::string_view defaultName  = "Unknown";
    const rttr::enumeration objectCategoryEnum  = get_object_category_enum();

    rttr::string_view name = objectCategoryEnum.value_to_name(objectCategory);
    return name.data() ? name : defaultName;
}

auto type_helper::get_object_from_instance(const rttr::instance& instance) -> sbk::core::object*
{
    return instance.try_convert<sbk::core::object>();
}

auto type_helper::get_database_object_from_instance(const rttr::instance& instance) -> sbk::core::database_object*
{
    return instance.try_convert<sbk::core::database_object>();
}

auto type_helper::get_node_from_instance(const rttr::instance& instance) -> sbk::engine::node*
{
    return instance.try_convert<sbk::engine::node>();
}

auto type_helper::get_node_base_from_instance(const rttr::instance& instance) -> sbk::engine::node_base*
{
    return instance.try_convert<sbk::engine::node_base>();
}