#include "memory.h"

#include "sound_bakery/core/object/object.h"
#include "sound_bakery/util/type_helper.h"

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
    void* pointer = std::malloc(size);
    TracyAllocN(pointer, size, sbk::util::type_helper::getObjectCategoryName(category).data());
    return pointer;
}

void* sbk::memory::realloc(void* pointer, std::size_t size) 
{ 
    return std::realloc(pointer, size); 
}

void sbk::memory::free(void* pointer, SB_OBJECT_CATEGORY category)
{
    TracyFreeN(pointer, sbk::util::type_helper::getObjectCategoryName(category).data());
    std::free(pointer);
}

