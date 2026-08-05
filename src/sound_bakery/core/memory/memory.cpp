#include "memory.h"

#include "sound_bakery/core/object/object.h"
#include "sound_bakery/system.h"
#include "sound_bakery/util/type_helper.h"

#include "rpmalloc/rpmalloc.h"

static std::size_t s_totalMemory = 0;

auto sbk::memory::init() -> void
{
    // rpmalloc_initialize is idempotent (guarded by rpmalloc's own initialized flag), so a
    // duplicate create() is safe.
    rpmalloc_initialize();
}

auto sbk::memory::shutdown() -> void
{
    rpmalloc_finalize();
}

// EASTL still declares these globals in its <EASTL/allocator.h>; we override EASTLAllocatorType
// so nothing here should be reached, but the definitions must exist to satisfy the link.
void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
    (void)pName;
    (void)flags;
    (void)debugFlags;
    (void)file;
    (void)line;
    return sbk::memory::malloc(size, sbk::memory::default_alignment, sbk::memory::object_category::data);
}

void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
    (void)alignmentOffset;
    (void)pName;
    (void)flags;
    (void)debugFlags;
    (void)file;
    (void)line;
    return sbk::memory::malloc(size, alignment, sbk::memory::object_category::data);
}

auto sbk::memory::default_eastl_allocator() noexcept -> polymorphic_allocator*
{
    // The single unavoidable static footprint of the EASTL integration: 16 bytes of a
    // stateless polymorphic_allocator whose m_resource is null. Every EASTL container that
    // default-constructs copies from this pointer, and the copy's allocate() then resolves
    // through system_default_resource() on each call -- so the "default" tracks the current
    // sbk::engine::system rather than being frozen at first use.
    static polymorphic_allocator instance;
    return &instance;
}

auto sbk::memory::system_default_resource() noexcept -> memory_resource*
{
    if (sbk::engine::system* const sys = sbk::engine::system::get())
    {
        return sys->get_default_memory_resource();
    }
    return nullptr;
}

auto sbk::memory::object_deleter::operator()(sbk::core::object* object)  const noexcept -> void
{
    if (object != nullptr)
    {
        const sbk::memory::object_category objectCategory = sbk::util::type_helper::get_category_from_type(object->get_object_type());

        if (sbk::engine::system* const system = object->get_system())
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

auto sbk::memory::malloc(std::size_t size, std::size_t alignment, sbk::memory::object_category category) -> void*
{
    ZoneScoped;

    // rpaligned_alloc short-circuits to the standard rpmalloc path when alignment <= 16, so
    // small-alignment callers pay no measurable cost. Anything more (SIMD, cache-line padded)
    // gets honoured for free rather than silently misaligned.
    void* allocatedPtr = rpaligned_alloc(alignment, size);

    if (allocatedPtr == nullptr)
    {
        // Central allocation choke point: rpmalloc returns null (it does not throw) on failure.
        // Log here; callers propagate the null (e.g. object creation returns an empty shared_ptr).
        sbk::log_error(SBK_ERR_OUT_OF_MEMORY, "rpmalloc failed to allocate the requested memory");
        return nullptr;
    }

    TracyAllocN(allocatedPtr, size, sbk::util::type_helper::get_object_category_name(category).data());
    s_totalMemory += size;
    return allocatedPtr;
}

auto sbk::memory::realloc(void* reallocPtr, std::size_t size) -> void*
{
    ZoneScoped;
    return rprealloc(reallocPtr, size);
}

auto sbk::memory::free(void* freePtr, sbk::memory::object_category category) -> void
{
    ZoneScoped;
    TracyFreeN(freePtr, sbk::util::type_helper::get_object_category_name(category).data());
    rpfree(freePtr);
}

auto sbk::memory::thread_start(std::string_view threadName) -> void
{
    rpmalloc_thread_initialize();
}

auto sbk::memory::thread_end(std::string_view threadName) -> void
{
    rpmalloc_thread_finalize(1);
}
