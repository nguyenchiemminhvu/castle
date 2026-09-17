// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file alignment.hpp
 * @brief Provides alignment checks and raw aligned storage building blocks.
 *
 * Use this header when code must verify pointer alignment or reserve compile-time-sized,
 * properly aligned bytes for later in-place construction. No dynamic allocation occurs,
 * but callers must ensure that any object placed into the storage fits, is sufficiently
 * aligned, and is explicitly destroyed when its lifetime ends.
 *
 * @code
 * using storage_type = castle::memory::aligned_storage_as_t<sizeof(uint32_t), uint32_t>;
 * storage_type storage;
 * uint32_t* pointer = storage.get_address<uint32_t>();
 * bool aligned = castle::memory::is_aligned<uint32_t>(pointer);
 * @endcode
 */
#ifndef CASTLE_MEMORY_MEMORY_HPP
#define CASTLE_MEMORY_MEMORY_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"

#include <stdint.h>

namespace castle
{
namespace memory
{

/**
 * @brief Checks whether a runtime pointer value satisfies a requested alignment.
 * @param p Pointer to inspect.
 * @param required_alignment Required byte alignment.
 * @return `true` when @p p is aligned to @p required_alignment; otherwise `false`.
 * @note Returns `false` when @p required_alignment is zero or not a power of two.
 * @warning This function checks the numeric address only; it does not verify that an object
 * of any particular type is alive at @p p.
 */
CASTLE_NODISCARD CASTLE_INLINE bool is_aligned(CASTLE_CONST void* p,
                                               size_type required_alignment) CASTLE_NOEXCEPT
{
    // LCOV_EXCL_START
    bool non_zero_alignment = required_alignment != 0U;
    bool power_of_two_alignment = (required_alignment & (required_alignment - 1U)) == 0U;

    bool pointer_aligned = false;
    if (non_zero_alignment) // Only evaluate the modulus when the divisor is valid.
    {
        pointer_aligned = (reinterpret_cast<uintptr_t>(p) % static_cast<uintptr_t>(required_alignment)) == 0U;
    }

    return non_zero_alignment && power_of_two_alignment && pointer_aligned;
    // LCOV_EXCL_STOP
}

/**
 * @brief Checks whether a pointer satisfies a compile-time alignment.
 * @tparam Alignment Required byte alignment. Must be a non-zero power of two.
 * @param p Pointer to inspect.
 * @return `true` when @p p is aligned to @p Alignment; otherwise `false`.
 * @note Invalid alignments are rejected with `static_assert`.
 * @warning This function checks the numeric address only; it does not verify object lifetime.
 */
template <size_type Alignment>
CASTLE_NODISCARD CASTLE_INLINE bool is_aligned(CASTLE_CONST void* p) CASTLE_NOEXCEPT
{
    static_assert((Alignment != 0U) && (Alignment & (Alignment - 1U)) == 0U,
                  "Alignment must be a power of two");
    return (reinterpret_cast<uintptr_t>(p) % static_cast<uintptr_t>(Alignment)) == 0U;
}

/**
 * @brief Checks whether a pointer satisfies `alignof(T)`.
 * @tparam T Type whose alignment requirement is used.
 * @param p Pointer to inspect.
 * @return `true` when @p p is aligned for `T`; otherwise `false`.
 * @note Equivalent to `is_aligned<alignof(T)>(p)`.
 * @warning Alignment alone does not mean a live `T` object exists at @p p.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_INLINE bool is_aligned(CASTLE_CONST void* p) CASTLE_NOEXCEPT
{
    return is_aligned<alignof(T)>(p);
}

/**
 * @brief Produces a type whose alignment is exactly `Alignment`.
 * @tparam Alignment Required byte alignment. Must be a non-zero power of two.
 * @note Useful for declaring sentinel objects or storage whose size is irrelevant but whose
 * alignment must match another component.
 * @warning The nested `type` contains only a dummy byte; it does not model a payload object.
 */
template <size_type Alignment>
struct type_with_alignment
{
    static_assert(Alignment != 0U, "Alignment must be non-zero");
    static_assert((Alignment & (Alignment - 1U)) == 0U,
                  "Alignment must be a power of two");

    /**
     * @brief Byte-sized placeholder with `Alignment` alignment.
     * @note This type is storage metadata only.
     * @warning Do not treat the `dummy` member as an aligned object of another type.
     */
    struct type
    {
        alignas(Alignment) unsigned char dummy;
    };
};

/**
 * @brief Convenience alias for `type_with_alignment<Alignment>::type`.
 * @tparam Alignment Required byte alignment.
 * @note Alias for declaring aligned placeholder objects directly.
 * @warning `Alignment` must be a non-zero power of two.
 */
template <size_type Alignment>
using type_with_alignment_t = typename type_with_alignment<Alignment>::type;

/**
 * @brief Provides raw storage of `Length` bytes at `Alignment` alignment.
 * @tparam Length Storage size in bytes. Must be non-zero.
 * @tparam Alignment Storage alignment in bytes. Must be a non-zero power of two.
 * @note This template reserves bytes only; it does not construct or destroy objects.
 * @warning Callers must ensure any object placed into the storage fits within `Length`,
 * satisfies `Alignment`, and has its lifetime managed explicitly.
 */
template <size_type Length, size_type Alignment>
struct aligned_storage
{
    static_assert(Length != 0U, "Length must be non-zero");
    static_assert(Alignment != 0U, "Alignment must be non-zero");
    static_assert((Alignment & (Alignment - 1U)) == 0U,
                  "Alignment must be a power of two");

    /**
     * @brief Concrete storage object containing the aligned bytes.
     * @note `data` does not imply that any object lifetime is active.
     * @warning Only form typed references after a compatible object has been constructed there.
     */
    struct type
    {
        alignas(Alignment) unsigned char data[Length];

        /**
         * @brief Returns the storage address as a pointer to `T`.
         * @tparam T Target type that will occupy the storage.
         * @return Typed pointer to the first byte of the storage.
         * @note Compile-time checks enforce that `T` fits and that the storage alignment is
         * compatible with `alignof(T)`.
         * @warning The returned pointer may refer to raw bytes until a live `T` object is
         * constructed in this storage.
         */
        template <typename T>
        CASTLE_NODISCARD CASTLE_INLINE T* get_address() CASTLE_NOEXCEPT
        {
            static_assert((Alignment % alignof(T)) == 0U,
                          "Incompatible alignment for target type");
            static_assert(sizeof(T) <= Length,
                          "Target type does not fit in aligned_storage");
            return reinterpret_cast<T*>(&data[0]);
        }

        /**
         * @brief Returns the storage address as a const pointer to `T`.
         * @tparam T Target type that will occupy the storage.
         * @return Const typed pointer to the first byte of the storage.
         * @note Compile-time checks enforce that `T` fits and that the storage alignment is
         * compatible with `alignof(T)`.
         * @warning The returned pointer may refer to raw bytes until a live `T` object is
         * constructed in this storage.
         */
        template <typename T>
        CASTLE_NODISCARD CASTLE_INLINE CASTLE_CONST T* get_address() CASTLE_CONST CASTLE_NOEXCEPT
        {
            static_assert((Alignment % alignof(T)) == 0U,
                          "Incompatible alignment for target type");
            static_assert(sizeof(T) <= Length,
                          "Target type does not fit in aligned_storage");
            return reinterpret_cast<CASTLE_CONST T*>(&data[0]);
        }

        /**
         * @brief Returns the storage as a mutable reference to `T`.
         * @tparam T Target type that already occupies the storage.
         * @return Reference to the object stored at the beginning of the buffer.
         * @note Equivalent to `*get_address<T>()`.
         * @warning A live `T` object must already exist in the storage before this is called.
         */
        template <typename T>
        CASTLE_NODISCARD CASTLE_INLINE T& get_reference() CASTLE_NOEXCEPT
        {
            return *get_address<T>();
        }

        /**
         * @brief Returns the storage as a const reference to `T`.
         * @tparam T Target type that already occupies the storage.
         * @return Const reference to the object stored at the beginning of the buffer.
         * @note Equivalent to `*get_address<T>()`.
         * @warning A live `T` object must already exist in the storage before this is called.
         */
        template <typename T>
        CASTLE_NODISCARD CASTLE_INLINE CASTLE_CONST T& get_reference() CASTLE_CONST CASTLE_NOEXCEPT
        {
            return *get_address<T>();
        }
    };
};

/**
 * @brief Convenience alias for `aligned_storage<Length, Alignment>::type`.
 * @tparam Length Storage size in bytes.
 * @tparam Alignment Storage alignment in bytes.
 * @note Use this alias when only the concrete storage object is needed.
 * @warning The alias still provides raw bytes only; object lifetime remains caller-managed.
 */
template <size_type Length, size_type Alignment>
using aligned_storage_t = typename aligned_storage<Length, Alignment>::type;

/**
 * @brief Adapts `aligned_storage` so the alignment is taken from `T`.
 * @tparam Length Storage size in bytes.
 * @tparam T Type whose alignment requirement is used.
 * @note Equivalent to `aligned_storage<Length, alignof(T)>`.
 * @warning Callers must still ensure that the chosen `Length` is large enough for the object
 * representation they place into the storage.
 */
template <size_type Length, typename T>
struct aligned_storage_as : aligned_storage<Length, alignof(T)>
{
};

/**
 * @brief Convenience alias for `aligned_storage_as<Length, T>::type`.
 * @tparam Length Storage size in bytes.
 * @tparam T Type whose alignment requirement is used.
 * @note This is often the simplest way to declare aligned raw storage for a single type.
 * @warning The alias reserves storage only; it does not start the lifetime of `T`.
 */
template <size_type Length, typename T>
using aligned_storage_as_t = typename aligned_storage_as<Length, T>::type;

}

/**
 * @brief Top-level alias for `castle::memory::type_with_alignment`.
 * @tparam Alignment Required byte alignment.
 * @note Provided for Castle's public alias pattern.
 * @warning `Alignment` must be a non-zero power of two.
 */
template <size_type Alignment>
using type_with_alignment = memory::type_with_alignment<Alignment>;

/**
 * @brief Top-level alias for `castle::memory::type_with_alignment_t`.
 * @tparam Alignment Required byte alignment.
 * @note Produces the aligned placeholder type directly.
 * @warning `Alignment` must be a non-zero power of two.
 */
template <size_type Alignment>
using type_with_alignment_t = memory::type_with_alignment_t<Alignment>;

/**
 * @brief Top-level alias for `castle::memory::aligned_storage`.
 * @tparam Length Storage size in bytes.
 * @tparam Alignment Storage alignment in bytes.
 * @note Provided for Castle's public alias pattern.
 * @warning The storage is raw and lifetime-neutral.
 */
template <size_type Length, size_type Alignment>
using aligned_storage = memory::aligned_storage<Length, Alignment>;

/**
 * @brief Top-level alias for `castle::memory::aligned_storage_t`.
 * @tparam Length Storage size in bytes.
 * @tparam Alignment Storage alignment in bytes.
 * @note Produces the concrete aligned storage object directly.
 * @warning The storage is raw and lifetime-neutral.
 */
template <size_type Length, size_type Alignment>
using aligned_storage_t = memory::aligned_storage_t<Length, Alignment>;

/**
 * @brief Top-level alias for `castle::memory::aligned_storage_as`.
 * @tparam Length Storage size in bytes.
 * @tparam T Type whose alignment requirement is used.
 * @note Equivalent to `castle::memory::aligned_storage_as<Length, T>`.
 * @warning The storage is raw and lifetime-neutral.
 */
template <size_type Length, typename T>
using aligned_storage_as = memory::aligned_storage_as<Length, T>;

/**
 * @brief Top-level alias for `castle::memory::aligned_storage_as_t`.
 * @tparam Length Storage size in bytes.
 * @tparam T Type whose alignment requirement is used.
 * @note Produces the concrete aligned storage object directly.
 * @warning The storage is raw and lifetime-neutral.
 */
template <size_type Length, typename T>
using aligned_storage_as_t = memory::aligned_storage_as_t<Length, T>;

}

#endif // CASTLE_MEMORY_MEMORY_HPP
