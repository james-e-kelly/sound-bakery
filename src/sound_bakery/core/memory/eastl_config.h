#pragma once

// Sound Bakery replaces EASTL's default allocator with sbk::memory::polymorphic_allocator
// (see allocator.h): a runtime-polymorphic allocator that wraps a sbk::memory::memory_resource*.
// The resource itself is resolved through sbk::engine::system, never through a process-global
// registry, so host applications cannot accidentally rebind us and we cannot rebind them.
//
// This header must be included before any EASTL header. It is included at the top of pch.h.
//
// The complete polymorphic_allocator definition is required (not just a forward declaration)
// because eastl::compressed_pair instantiates eastl::is_empty on the allocator type when
// EASTL container templates are processed.

#include "sound_bakery/core/memory/allocator.h"

#define EASTLAllocatorType    sbk::memory::polymorphic_allocator
#define EASTLAllocatorDefault sbk::memory::default_eastl_allocator
// Do NOT override the per-container defaults like EASTL_VECTOR_DEFAULT_ALLOCATOR. EASTL's own
// definition (`allocator_type(EASTL_VECTOR_DEFAULT_NAME)`) is an expression that constructs an
// allocator from a name -- overriding it with just a type name breaks noexcept-expr contexts.
// polymorphic_allocator has a `(const char*)` constructor so the default expansion Just Works.
