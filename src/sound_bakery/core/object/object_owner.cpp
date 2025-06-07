#include "object_owner.h"

#include "sound_bakery/core/memory.h"
#include "sound_bakery/serialization/serializer.h"
#include "sound_bakery/system.h"
#include "sound_bakery/util/type_helper.h"

auto sbk::core::object_owner::create_runtime_object(const rttr::type& type) -> concurrencpp::result<std::shared_ptr<sbk::core::object>>
{
    sbk::engine::system* const system = sbk::engine::system::get();

    if (!system)
    {
        SBK_CRITICAL("Cannot create object. System is invalid");
        co_return std::shared_ptr<sbk::core::object>{};
    }

    if (!type.is_valid())
    {
        SBK_ERROR("Cannot create object. Object type is invalid");
        co_return std::shared_ptr<sbk::core::object>{};
    }

    if (!type.is_derived_from(rttr::type::get<sbk::core::object>()))
    {
        SBK_ERROR("Cannot create object. Objects must derive from the base object type");
        co_return std::shared_ptr<sbk::core::object>{};
    }

    const rttr::constructor constructor = type.get_constructor();

    if (!constructor.is_valid())
    {
        SBK_ERROR("Objects in Sound Bakery must be constructable. Define this in the reflection file");
        co_return std::shared_ptr<sbk::core::object>{};
    }

    const rttr::variant variant = constructor.invoke();

    if (!variant.is_valid() || !variant.get_type().is_valid() || !variant.get_type().get_raw_type().is_valid())
    {
        SBK_ERROR("Failed to create object");
        co_return std::shared_ptr<sbk::core::object>{};
    }

    BOOST_ASSERT(variant.get_type().is_pointer());

    std::shared_ptr<sbk::core::object> result;
    
    // If raw
    if (variant.get_type().is_pointer())
    {
        result = std::shared_ptr<sbk::core::object>(variant.convert<sbk::core::object*>(), sbk::memory::object_deleter());
    }
    else
    {
        BOOST_ASSERT(false);
    }

    BOOST_ASSERT(result);

    co_await add_reference_to_object(result);

    co_await system->track_object(result.get());

    result->set_owner(this);

    result->cache_type();

    co_return result;
}

auto sbk::core::object_owner::create_database_object(const rttr::type& type,
                                                     bool addToDatabase) -> concurrencpp::result<std::shared_ptr<sbk::core::database_object>>
{
    sbk::engine::system* const system = sbk::engine::system::get();

    if (system == nullptr)
    {
        SBK_CRITICAL("Cannot create object. System is invalid");
        co_return std::shared_ptr<sbk::core::database_object>{};
    }

    if (!type.is_derived_from(rttr::type::get<sbk::core::database_object>()))
    {
        SBK_ERROR("Cannot create object. Database objects must derive from the base database object type");
        co_return std::shared_ptr<sbk::core::database_object>{};
    }

    if (const std::shared_ptr<object> object = co_await create_runtime_object(type))
    {
        if (std::shared_ptr<database_object> databaseObject = std::static_pointer_cast<database_object>(object))
        {
            if (addToDatabase)
            {
                co_await system->add_object_to_database(databaseObject);
            }

            co_return databaseObject;
        }
    }

    co_return std::shared_ptr<sbk::core::database_object>{};
}

auto sbk::core::object_owner::add_reference_to_object(std::shared_ptr<object> object) -> concurrencpp::result<void>
{
    if (object)
    {
        const concurrencpp::scoped_async_lock objectLock = co_await get_object_lock();
        m_objects.push_back(object);
    }
}

auto sbk::core::object_owner::remove_reference_to_object(std::shared_ptr<object> object) -> concurrencpp::result<void>
{
    if (object)
    {
        const concurrencpp::scoped_async_lock objectLock = co_await get_object_lock();

        for (std::vector<std::shared_ptr<sbk::core::object>>::iterator iter = m_objects.begin();
             iter != m_objects.end(); ++iter)
        {
            if (*iter == object)
            {
                m_objects.erase(iter);
                co_return;
            }
        }
    }
    co_return;
}

auto sbk::core::object_owner::remove_all() -> concurrencpp::result<void> 
{ 
    const concurrencpp::scoped_async_lock objectLock = co_await get_object_lock();
    m_objects.clear(); 
}

auto sbk::core::object_owner::get_referenced_objects() const -> concurrencpp::result<std::vector<std::shared_ptr<object>>> 
{ 
    const concurrencpp::scoped_async_lock objectLock = co_await get_object_lock();
    co_return m_objects; 
}

auto sbk::core::object_owner::get_referenced_object_at(std::size_t index) const -> concurrencpp::result<std::shared_ptr<object>>
{
    const concurrencpp::scoped_async_lock objectLock = co_await get_object_lock();

    if (index >= 0 && index < m_objects.size())
    {
        co_return m_objects.at(index);
    }
    co_return std::shared_ptr<object>{};
}

auto sbk::core::object_owner::get_referenced_objects_size() const -> concurrencpp::result<std::size_t>
{ 
    const concurrencpp::scoped_async_lock objectLock = co_await get_object_lock();
    co_return m_objects.size(); 
}

auto sbk::core::object_owner::get_object_lock() const -> concurrencpp::lazy_result<concurrencpp::scoped_async_lock>
{
    co_return co_await m_objectsLock.lock(sbk::engine::system::get()->get_background_thread_executor());
}