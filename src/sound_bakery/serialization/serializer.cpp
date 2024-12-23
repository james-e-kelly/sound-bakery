#include "serializer.h"

#include "sound_bakery/core/object/object.h"
#include "sound_bakery/core/object/object_tracker.h"
#include "sound_bakery/event/event.h"
#include "sound_bakery/node/container/sound_container.h"
#include "sound_bakery/node/node.h"
#include "sound_bakery/sound/sound.h"
#include "sound_bakery/soundbank/soundbank.h"
#include "sound_bakery/system.h"

auto sbk::core::serialization::make_default_variant(const rttr::type& type) -> rttr::variant
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
    else if (type.is_arithmetic())  // Can't construct arithmetic types
    {
        rttr::variant variant = 0u;
        variant.convert(type);
        BOOST_ASSERT_MSG(variant.is_valid(), "Could not convert the default value to type");
        return variant;
    }
    else if (type.is_enumeration())
    {
        rttr::enumeration enumeration = type.get_enumeration();
        rttr::array_range<rttr::variant> values = enumeration.get_values();
        BOOST_ASSERT_MSG(!values.empty(), "Enums must have values so we can create a default one");
        return *values.begin();
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

auto sbk::core::serialization::read_binary_file(const std::filesystem::path& file) -> std::vector<uint8_t> 
{
    std::ifstream fileStream(file);

    fileStream.seekg(0, std::ios_base::end);
    std::streampos length = fileStream.tellg();
    fileStream.seekg(0, std::ios_base::beg);

    std::vector<uint8_t> buffer(length);
    fileStream.read((char*)buffer.data(), length);

    return buffer;
}