#ifndef CASTLE_MEMORY_STORAGE_H
#define CASTLE_MEMORY_STORAGE_H

#include "castle/core/compiler.h"

#include <stddef.h>
#include <stdint.h>

namespace castle
{
namespace memory
{

// Raw storage is storage only. It does not imply that a T object exists.
template <typename T, size_t N>
class raw_storage
{
    static_assert(N > 0U, "raw_storage requires a positive capacity");
    static_assert(sizeof(T) > 0U, "T must be a complete object type");

public:
    using value_type = T;
    static CASTLE_CONSTEXPR size_t capacity = N;

    void* address(size_t index) CASTLE_NOEXCEPT
    {
        return static_cast<void*>(data_ + (index * sizeof(T)));
    }

    CASTLE_CONST void* address(size_t index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<CASTLE_CONST void*>(data_ + (index * sizeof(T)));
    }

    uint8_t* bytes() CASTLE_NOEXCEPT { return data_; }
    CASTLE_CONST uint8_t* bytes() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }

private:
    alignas(T) uint8_t data_[sizeof(T) * N];
};

template <typename T>
class raw_storage<T, 0U>
{
public:
    using value_type = T;
    static CASTLE_CONSTEXPR size_t capacity = 0U;

    void* address(size_t) CASTLE_NOEXCEPT { return nullptr; }
    CASTLE_CONST void* address(size_t) CASTLE_CONST CASTLE_NOEXCEPT { return nullptr; }
    uint8_t* bytes() CASTLE_NOEXCEPT { return nullptr; }
    CASTLE_CONST uint8_t* bytes() CASTLE_CONST CASTLE_NOEXCEPT { return nullptr; }
};

} // namespace memory
} // namespace castle

#endif // CASTLE_MEMORY_STORAGE_H
