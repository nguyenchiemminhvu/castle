#ifndef CASTLE_MEMORY_NEW_HPP
#define CASTLE_MEMORY_NEW_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"

#include <stddef.h>

#if CASTLE_USING_STD_NEW
    #include <new>
#else
    CASTLE_INLINE void* operator new(size_t, void* p) CASTLE_NOEXCEPT
    {
        return p;
    }

    CASTLE_INLINE void* operator new[](size_t, void* p) CASTLE_NOEXCEPT
    {
        return p;
    }

    CASTLE_INLINE void operator delete(void*, void*) CASTLE_NOEXCEPT {}
    CASTLE_INLINE void operator delete[](void*, void*) CASTLE_NOEXCEPT {}
#endif

#endif // CASTLE_MEMORY_NEW_HPP
