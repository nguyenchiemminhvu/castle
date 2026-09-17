// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file new.hpp
 * @brief Supplies placement-new support without introducing heap allocation requirements.
 *
 * Use this header before placement construction in environments where `<new>` may or may not
 * be available. When the standard header exists it is included; otherwise Castle provides the
 * placement `new` and matching placement `delete` operators needed by in-place construction.
 *
 * @code
 * alignas(uint32_t) unsigned char bytes[sizeof(uint32_t)];
 * uint32_t* value = ::new (static_cast<void*>(bytes)) uint32_t(7U);
 * @endcode
 */
#ifndef CASTLE_MEMORY_NEW_HPP
#define CASTLE_MEMORY_NEW_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"

#include <stddef.h>

#if CASTLE_USING_STD_NEW
    #include <new>
#else
    /**
     * @brief Placement `new` returning the supplied storage address.
     * @param p Caller-provided storage.
     * @return `p`.
     * @note This overload performs no allocation.
     * @warning `p` must point to storage that is suitably aligned and large enough for the
     * object being constructed.
     */
    CASTLE_INLINE void* operator new(size_t, void* p) CASTLE_NOEXCEPT
    {
        return p;
    }

    /**
     * @brief Placement array `new` returning the supplied storage address.
     * @param p Caller-provided storage.
     * @return `p`.
     * @note This overload performs no allocation.
     * @warning `p` must point to storage that is suitably aligned and large enough for the
     * array being constructed.
     */
    CASTLE_INLINE void* operator new[](size_t, void* p) CASTLE_NOEXCEPT
    {
        return p;
    }

    /**
     * @brief Matching placement `delete` overload for failed scalar construction.
     * @param Unused pointer value required by the language.
     * @param Unused placement address.
     * @return Nothing.
     * @note This function is intentionally empty because placement construction does not allocate.
     * @warning This overload is not a general deallocation function.
     */
    CASTLE_INLINE void operator delete(void*, void*) CASTLE_NOEXCEPT {}

    /**
     * @brief Matching placement `delete[]` overload for failed array construction.
     * @param Unused pointer value required by the language.
     * @param Unused placement address.
     * @return Nothing.
     * @note This function is intentionally empty because placement construction does not allocate.
     * @warning This overload is not a general deallocation function.
     */
    CASTLE_INLINE void operator delete[](void*, void*) CASTLE_NOEXCEPT {}
#endif

#endif // CASTLE_MEMORY_NEW_HPP
