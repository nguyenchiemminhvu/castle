#ifndef CASTLE_MEMORY_MEMORY_H
#define CASTLE_MEMORY_MEMORY_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"
#include "castle/core/types.h"

#include <stdint.h>

namespace castle
{
namespace memory
{

// ---------------------------------------------------------------------------
// is_aligned
// ---------------------------------------------------------------------------
// Runtime check: pointer address is a multiple of the requested alignment.
// The uintptr_t round-trip is the only portable way to reason about pointer
// numeric value without triggering undefined behavior on arithmetic.

CASTLE_NODISCARD CASTLE_INLINE bool is_aligned(CASTLE_CONST void* p,
                                               size_type required_alignment) CASTLE_NOEXCEPT
{
    return ((required_alignment != 0U) && (required_alignment & (required_alignment - 1U)) == 0U)
        && (reinterpret_cast<uintptr_t>(p) % static_cast<uintptr_t>(required_alignment)) == 0U;
}

template <size_type Alignment>
CASTLE_NODISCARD CASTLE_INLINE bool is_aligned(CASTLE_CONST void* p) CASTLE_NOEXCEPT
{
    static_assert((Alignment != 0U) && (Alignment & (Alignment - 1U)) == 0U,
                  "Alignment must be a power of two");
    return (reinterpret_cast<uintptr_t>(p) % static_cast<uintptr_t>(Alignment)) == 0U;
}

template <typename T>
CASTLE_NODISCARD CASTLE_INLINE bool is_aligned(CASTLE_CONST void* p) CASTLE_NOEXCEPT
{
    return is_aligned<alignof(T)>(p);
}

// ---------------------------------------------------------------------------
// type_with_alignment
// ---------------------------------------------------------------------------
// Produces a POD type with exactly the requested alignment. In C++17 the
// language guarantees alignas() honors any fundamental alignment, so a single
// alignas'd byte is sufficient. This avoids ETL's compile-time type search.

template <size_type Alignment>
struct type_with_alignment
{
    static_assert(Alignment != 0U, "Alignment must be non-zero");
    static_assert((Alignment & (Alignment - 1U)) == 0U,
                  "Alignment must be a power of two");

    struct type
    {
        alignas(Alignment) unsigned char dummy;
    };
};

template <size_type Alignment>
using type_with_alignment_t = typename type_with_alignment<Alignment>::type;

// ---------------------------------------------------------------------------
// aligned_storage
// ---------------------------------------------------------------------------
// Raw byte buffer of Length bytes at Alignment alignment. Callers construct
// their object in-place with placement new via get_address<T>() and destroy
// it explicitly. No lifetime tracking is performed; higher-level wrappers
// (typed_storage, inplace_function) layer that on top.

template <size_type Length, size_type Alignment>
struct aligned_storage
{
    static_assert(Length != 0U, "Length must be non-zero");
    static_assert(Alignment != 0U, "Alignment must be non-zero");
    static_assert((Alignment & (Alignment - 1U)) == 0U,
                  "Alignment must be a power of two");

    struct type
    {
        alignas(Alignment) unsigned char data[Length];

        // Access helpers are templated on the destination type so a single
        // storage instance can back several logically-distinct views. The
        // alignment compatibility check catches misuse at compile time.
        template <typename T>
        CASTLE_NODISCARD CASTLE_INLINE T* get_address() CASTLE_NOEXCEPT
        {
            static_assert((Alignment % alignof(T)) == 0U,
                          "Incompatible alignment for target type");
            static_assert(sizeof(T) <= Length,
                          "Target type does not fit in aligned_storage");
            return reinterpret_cast<T*>(&data[0]);
        }

        template <typename T>
        CASTLE_NODISCARD CASTLE_INLINE CASTLE_CONST T* get_address() CASTLE_CONST CASTLE_NOEXCEPT
        {
            static_assert((Alignment % alignof(T)) == 0U,
                          "Incompatible alignment for target type");
            static_assert(sizeof(T) <= Length,
                          "Target type does not fit in aligned_storage");
            return reinterpret_cast<CASTLE_CONST T*>(&data[0]);
        }

        template <typename T>
        CASTLE_NODISCARD CASTLE_INLINE T& get_reference() CASTLE_NOEXCEPT
        {
            return *get_address<T>();
        }

        template <typename T>
        CASTLE_NODISCARD CASTLE_INLINE CASTLE_CONST T& get_reference() CASTLE_CONST CASTLE_NOEXCEPT
        {
            return *get_address<T>();
        }
    };
};

template <size_type Length, size_type Alignment>
using aligned_storage_t = typename aligned_storage<Length, Alignment>::type;

// ---------------------------------------------------------------------------
// aligned_storage_as
// ---------------------------------------------------------------------------
// Convenience: derive the alignment from an example type T. Length is still
// expressed in bytes so the storage can hold something other than a single
// T (for example, a small buffer holding several small callables).

template <size_type Length, typename T>
struct aligned_storage_as : aligned_storage<Length, alignof(T)>
{
};

template <size_type Length, typename T>
using aligned_storage_as_t = typename aligned_storage_as<Length, T>::type;

} // namespace memory

// Public aliases follow the same pattern established by castle/core/traits.h:
// the implementation lives inside a nested namespace, and user-facing code
// gets short names in the top-level castle namespace.
template <size_type Alignment>
using type_with_alignment = memory::type_with_alignment<Alignment>;
template <size_type Alignment>
using type_with_alignment_t = memory::type_with_alignment_t<Alignment>;

template <size_type Length, size_type Alignment>
using aligned_storage = memory::aligned_storage<Length, Alignment>;
template <size_type Length, size_type Alignment>
using aligned_storage_t = memory::aligned_storage_t<Length, Alignment>;

template <size_type Length, typename T>
using aligned_storage_as = memory::aligned_storage_as<Length, T>;
template <size_type Length, typename T>
using aligned_storage_as_t = memory::aligned_storage_as_t<Length, T>;

} // namespace castle

#endif // CASTLE_MEMORY_MEMORY_H