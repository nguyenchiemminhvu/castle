// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file storage.hpp
 * @brief Provides fixed-capacity raw storage indexed in `T`-sized slots.
 *
 * Use this header to reserve deterministic, heap-free bytes for later in-place construction.
 * The storage only guarantees size and alignment; it does not start object lifetime, track
 * initialization, or perform destruction automatically.
 *
 * @code
 * castle::memory::raw_storage<uint32_t, 2U> storage;
 * void* first_slot = storage.address(0U);
 * @endcode
 */
#ifndef CASTLE_MEMORY_STORAGE_HPP
#define CASTLE_MEMORY_STORAGE_HPP

#include "castle/core/compiler.hpp"

#include <stddef.h>
#include <stdint.h>

namespace castle
{
namespace memory
{

/**
 * @brief Raw storage containing `N` contiguous slots sized and aligned for `T`.
 * @tparam T Type whose size and alignment define each slot.
 * @tparam N Number of slots in the storage.
 * @note The storage contains bytes only until objects are explicitly constructed there.
 * @warning Indices passed to `address()` must be less than `capacity`, and any object created
 * in a slot must be destroyed explicitly before that slot is reused for an incompatible object.
 */
template <typename T, size_t N>
class raw_storage
{
    static_assert(N > 0U, "raw_storage requires a positive capacity");
    static_assert(sizeof(T) > 0U, "T must be a complete object type");

public:
    /** @brief Type whose size and alignment define each slot. */
    using value_type = T;

    /** @brief Number of available slots. */
    static CASTLE_CONSTEXPR size_t capacity = N;

    /**
     * @brief Returns the starting address of slot `index`.
     * @param index Zero-based slot index.
     * @return Untyped pointer to the requested slot.
     * @note The returned address is aligned for `T`.
     * @warning `index` must be less than `capacity`, and the returned pointer names raw storage
     * until a live object is constructed there.
     */
    void* address(size_t index) CASTLE_NOEXCEPT
    {
        return static_cast<void*>(data_ + (index * sizeof(T)));
    }

    /**
     * @brief Returns the starting address of slot `index` as a const pointer.
     * @param index Zero-based slot index.
     * @return Const untyped pointer to the requested slot.
     * @note The returned address is aligned for `T`.
     * @warning `index` must be less than `capacity`, and the returned pointer names raw storage
     * until a live object is constructed there.
     */
    CASTLE_CONST void* address(size_t index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<CASTLE_CONST void*>(data_ + (index * sizeof(T)));
    }

    /**
     * @brief Returns the underlying byte buffer.
     * @return Pointer to the first storage byte.
     * @note Useful for serialization or low-level transport of the raw storage block.
     * @warning Treat the returned pointer as bytes unless an object's lifetime is known to be active.
     */
    uint8_t* bytes() CASTLE_NOEXCEPT { return data_; }

    /**
     * @brief Returns the underlying byte buffer as a const pointer.
     * @return Const pointer to the first storage byte.
     * @note Useful for serialization or low-level transport of the raw storage block.
     * @warning Treat the returned pointer as bytes unless an object's lifetime is known to be active.
     */
    CASTLE_CONST uint8_t* bytes() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }

private:
    alignas(T) uint8_t data_[sizeof(T) * N];
};

/**
 * @brief Zero-capacity specialization of `raw_storage`.
 * @tparam T Type whose size and alignment would define each slot.
 * @note This specialization avoids allocating any backing bytes.
 * @warning All accessors return `nullptr`; no object may be constructed in this storage.
 */
template <typename T>
class raw_storage<T, 0U>
{
public:
    /** @brief Type whose size and alignment would define each slot. */
    using value_type = T;

    /** @brief Number of available slots, always zero. */
    static CASTLE_CONSTEXPR size_t capacity = 0U;

    /**
     * @brief Returns `nullptr` because no slots exist.
     * @param Unused slot index.
     * @return `nullptr`.
     * @note Provided so zero-capacity storage shares the same API shape.
     * @warning The returned pointer cannot be dereferenced or used for construction.
     */
    void* address(size_t) CASTLE_NOEXCEPT { return nullptr; }

    /**
     * @brief Returns `nullptr` because no slots exist.
     * @param Unused slot index.
     * @return `nullptr`.
     * @note Provided so zero-capacity storage shares the same API shape.
     * @warning The returned pointer cannot be dereferenced or used for construction.
     */
    CASTLE_CONST void* address(size_t) CASTLE_CONST CASTLE_NOEXCEPT { return nullptr; }

    /**
     * @brief Returns `nullptr` because no storage bytes exist.
     * @return `nullptr`.
     * @note Provided so zero-capacity storage shares the same API shape.
     * @warning The returned pointer cannot be dereferenced or used for construction.
     */
    uint8_t* bytes() CASTLE_NOEXCEPT { return nullptr; }

    /**
     * @brief Returns `nullptr` because no storage bytes exist.
     * @return `nullptr`.
     * @note Provided so zero-capacity storage shares the same API shape.
     * @warning The returned pointer cannot be dereferenced or used for construction.
     */
    CASTLE_CONST uint8_t* bytes() CASTLE_CONST CASTLE_NOEXCEPT { return nullptr; }
};

}
}

#endif // CASTLE_MEMORY_STORAGE_HPP
