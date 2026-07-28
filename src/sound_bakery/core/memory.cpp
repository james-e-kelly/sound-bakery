#include "memory.h"

#include "sound_bakery/core/object/object.h"
#include "sound_bakery/error/error.h"
#include "sound_bakery/system.h"
#include "sound_bakery/util/type_helper.h"

#include "rpmalloc/rpmalloc.h"

static std::size_t s_totalMemory = 0;

struct rpmalloc_wrapper
{
    rpmalloc_wrapper()
    {
        rpmalloc_initialize();
    }

    ~rpmalloc_wrapper()
    {
        rpmalloc_finalize();
    }
};

static rpmalloc_wrapper s_rpmalloc;

auto sbk::memory::object_deleter::operator()(sbk::core::object* object)  const noexcept -> void
{
    if (object != nullptr)
    {
        const SB_OBJECT_CATEGORY objectCategory = sbk::util::type_helper::get_category_from_type(object->get_object_type());

        if (sbk::engine::system* const system = sbk::engine::system::get())
        {
            if (const sbk::core::database_object* const databaseObject = sbk::cast<sbk::core::database_object*>(object))
            {
                if (const sbk_id id = databaseObject->get_database_id(); id != SBK_INVALID_ID)
                {
                    (void)system->remove_object_from_database(id);
                }
                else
                {
                    BOOST_ASSERT(databaseObject->has_flag(sbk::core::object_flags::default_name));
                }
            }

            system->untrack_object(object);
        }

        object->~object();
        sbk::memory::free(object, objectCategory);
    }
}

auto sbk::memory::malloc(std::size_t size, SB_OBJECT_CATEGORY category) -> void*
{
    void* pointer = rpmalloc(size);

    if (pointer == nullptr)
    {
        // Central allocation choke point: rpmalloc returns null (it does not throw) on failure.
        // Log here; callers propagate the null (e.g. object creation returns an empty shared_ptr).
        sbk::log_error(SBK_ERR_OUT_OF_MEMORY, "rpmalloc failed to allocate the requested memory");
        return nullptr;
    }

    TracyAllocN(pointer, size, sbk::util::type_helper::get_object_category_name(category).data());
    s_totalMemory += size;
    return pointer;
}

auto sbk::memory::realloc(void* pointer, std::size_t size) -> void*
{
    return rprealloc(pointer, size);
}

auto sbk::memory::free(void* pointer, SB_OBJECT_CATEGORY category) -> void
{
    TracyFreeN(pointer, sbk::util::type_helper::get_object_category_name(category).data());
    rpfree(pointer);
}

auto sbk::memory::thread_start(std::string_view threadName) -> void
{
    rpmalloc_thread_initialize();
}

auto sbk::memory::thread_end(std::string_view threadName) -> void
{
    rpmalloc_thread_finalize(1);
}