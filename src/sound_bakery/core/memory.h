#pragma once

#include "sound_bakery/pch.h"

namespace sbk::core
{
    class object;
}

namespace sbk::memory
{
    struct object_deleter
    {
        void operator()(sbk::core::object* object);
    };

    void* malloc(std::size_t size, SB_OBJECT_CATEGORY category);
    void* realloc(void* pointer, std::size_t size);
    void free(void* pointer, SB_OBJECT_CATEGORY category);

    auto thread_start(std::string_view threadName) -> void;
    auto thread_end(std::string_view threadName) -> void;
}