#pragma once

// Sound Bakery replaces EASTL's default allocator with sbk::memory::polymorphic_allocator
// (see memory.h): a runtime-polymorphic allocator that wraps a sbk::memory::memory_resource*.
// The resource itself is resolved through sbk::engine::system, never through a process-global
// registry, so host applications cannot accidentally rebind us and we cannot rebind them.
//
// This header must be included before any EASTL header. It is included at the top of pch.h.

namespace sbk::memory
{
    class polymorphic_allocator;

    // Returns a process-local default polymorphic_allocator. The returned allocator's
    // resource is nullptr; it resolves lazily on each allocate() via sbk::engine::system.
    // Called by EASTL container default constructors.
    auto default_eastl_allocator() noexcept -> polymorphic_allocator*;
}

#define EASTLAllocatorType    sbk::memory::polymorphic_allocator
#define EASTLAllocatorDefault sbk::memory::default_eastl_allocator
// Do NOT override the per-container defaults like EASTL_VECTOR_DEFAULT_ALLOCATOR. EASTL's own
// definition (`allocator_type(EASTL_VECTOR_DEFAULT_NAME)`) is an expression that constructs an
// allocator from a name -- overriding it with just a type name breaks noexcept-expr contexts.
// polymorphic_allocator has a `(const char*)` constructor so the default expansion Just Works.
