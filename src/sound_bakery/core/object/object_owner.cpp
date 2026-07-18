#include "object_owner.h"

#include "sound_bakery/core/memory.h"
#include "sound_bakery/error/result.h"
#include "sound_bakery/serialization/serializer.h"
#include "sound_bakery/system.h"
#include "sound_bakery/util/type_helper.h"

auto sbk::core::object_owner::create_runtime_object(const rttr::type& type) -> sbk::result<std::shared_ptr<sbk::core::object>>
{
    sbk::engine::system* const system = sbk::engine::system::get();

    SBK_CHECK(system != nullptr, SBK_ERR_BAKERY_UNINITIALIZED);
    SBK_CHECK_MSG(type.is_valid(), SBK_ERR_INVALID_PARAMETER, "Type was invalid. Ensure calling code supplies a valid type");
    BOOST_ASSERT_MSG(type.is_derived_from(rttr::type::get<sbk::core::object>()), "Object owners can only create objects that inherit sbk::core::object");

    const rttr::constructor constructor = type.get_constructor();

    BOOST_ASSERT_MSG(constructor.is_valid(), "Objects in Sound Bakery must be constructable. Define this in the reflection file");

    try
    {
        const rttr::variant variant = constructor.invoke();

        SBK_CHECK_MSG(variant.is_valid(), SBK_ERR_BAKERY, "Failed to create object. Variant is invalid");
        BOOST_ASSERT(variant.get_type().is_valid());
        BOOST_ASSERT(variant.get_type().get_raw_type().is_valid());
        BOOST_ASSERT_MSG(variant.get_type().is_pointer(), "Objects in Sound Bakery must be constructed on the heap and be pointers");

        sbk::core::object* const rawObject = variant.convert<sbk::core::object*>();
        std::shared_ptr<sbk::core::object> result(rawObject, sbk::memory::object_deleter());

        m_objects.emplace_back(result);
        system->track_object(result.get());
        result->set_owner(this);
        result->cache_type();

        return result;
    }
    catch (const std::exception& exception)
    {
        // All object allocation funnels through here, so this is the one place we guard against
        // exceptions thrown while constructing an object or growing our bookkeeping (e.g. std::bad_alloc).
        return sbk::make_error(SBK_ERR_OUT_OF_MEMORY, exception.what());
    }

    return sbk::make_error(SBK_ERR_BAKERY);
}

auto sbk::core::object_owner::create_database_object(const rttr::type& type, bool addToDatabase) -> sbk::result<std::shared_ptr<sbk::core::database_object>>
{
    sbk::engine::system* const system = sbk::engine::system::get();

    SBK_CHECK(system != nullptr, SBK_ERR_BAKERY_UNINITIALIZED);
    SBK_CHECK_MSG(type.is_valid(), SBK_ERR_INVALID_PARAMETER, "Type was invalid. Ensure calling code supplies a valid type");
    SBK_CHECK_MSG(type.is_derived_from(rttr::type::get<sbk::core::database_object>()), SBK_ERR_INVALID_PARAMETER, "Cannot create object. Database objects must derive from the base database object type");

    try
    {
        SBK_TRY(auto object, create_runtime_object(type));

        if (std::shared_ptr<database_object> databaseObject = std::static_pointer_cast<database_object>(object))
        {
            if (addToDatabase)
            {
                system->add_object_to_database(databaseObject);
            }

            return databaseObject;
        }
    }
    catch (const std::exception& exception)
    {
        return sbk::make_error(SBK_ERR_OUT_OF_MEMORY, exception.what());
    }

    return sbk::make_error(SBK_ERR_BAKERY);
}

auto sbk::core::object_owner::add_reference_to_object(std::shared_ptr<database_object>& object) -> void
{
    if (object)
    {
        m_objects.push_back(object);
    }
}

auto sbk::core::object_owner::remove_object(const std::shared_ptr<object>& object) -> std::vector<std::shared_ptr<sbk::core::object>>::iterator
{
    if (object)
    {
        for (auto iter = m_objects.begin(); iter != m_objects.end(); ++iter)
        {
            if (*iter == object)
            {
                return m_objects.erase(iter);
            }
        }
    }
    return m_objects.end();
}

auto sbk::core::object_owner::remove_all() -> void { m_objects.clear(); }

auto sbk::core::object_owner::get_objects() -> std::vector<std::shared_ptr<object>>& { return m_objects; }

auto sbk::core::object_owner::get_objects() const -> const std::vector<std::shared_ptr<object>>& { return m_objects; }

auto sbk::core::object_owner::get_objects_size() const -> std::size_t { return m_objects.size(); }
