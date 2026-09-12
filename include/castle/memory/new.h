#ifndef CASTLE_MEMORY_NEW_H
#define CASTLE_MEMORY_NEW_H

#include "castle/core/compiler.h"

#include <stddef.h>

#if !defined(CASTLE_USING_STD_NEW)
    #if defined(__has_include)
        #if __has_include(<new>)
            #define CASTLE_USING_STD_NEW 1
        #else
            #define CASTLE_USING_STD_NEW 0
        #endif
    #else
        #if defined(__STDC_HOSTED__) && (__STDC_HOSTED__ == 1)
            #define CASTLE_USING_STD_NEW 1
        #else
            #define CASTLE_USING_STD_NEW 0
        #endif
    #endif
#endif // !defined(CASTLE_USING_STD_NEW)

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

#endif // CASTLE_MEMORY_NEW_H
