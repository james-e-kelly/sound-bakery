#include "memory.h"

#include "sound_bakery/core/object/object.h"
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

void sbk::memory::object_deleter::operator()(sbk::core::object* object)
{
    if (object)
    {
        const SB_OBJECT_CATEGORY objectCategory = sbk::util::type_helper::getCategoryFromType(object->get_object_type());
        object->~object();
        sbk::memory::free(object, objectCategory);
    }
}

void* sbk::memory::malloc(std::size_t size, SB_OBJECT_CATEGORY category)
{
    void* pointer = rpmalloc(size);
    TracyAllocN(pointer, size, sbk::util::type_helper::getObjectCategoryName(category).data());
    s_totalMemory += size;
    return pointer;
}

void* sbk::memory::realloc(void* pointer, std::size_t size) 
{ 
    return rprealloc(pointer, size); 
}

void sbk::memory::free(void* pointer, SB_OBJECT_CATEGORY category)
{
    TracyFreeN(pointer, sbk::util::type_helper::getObjectCategoryName(category).data());
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