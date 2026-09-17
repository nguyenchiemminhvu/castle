#ifndef CASTLE_MEMORY_SOO_BUFFER_H
#define CASTLE_MEMORY_SOO_BUFFER_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"
#include "castle/core/types.h"
#include "castle/utility/utility.h"
#include "castle/memory/lifetime.h"
#include "castle/memory/new.h"

#include <stdint.h>

namespace castle
{
namespace memory
{

template <typename T, size_type StackSize = sizeof(T)>
class soo_buffer
{
public:
    template <typename Arg, typename = meta::enable_if_t<sizeof(T) <= StackSize, void>>
    soo_buffer(Arg&& arg) CASTLE_NOEXCEPT
    {
        ::new (static_cast<void*>(inline_buffer)) Arg(CASTLE_FORWARD<Arg>(arg));
    }

    soo_buffer(CASTLE_CONST soo_buffer&) CASTLE_DELETE;
    soo_buffer& operator=(CASTLE_CONST soo_buffer&) CASTLE_DELETE;

    soo_buffer(soo_buffer&& other) CASTLE_NOEXCEPT
    {
        this->~soo_buffer(); // Clean up existing resource
        ::new (static_cast<void*>(inline_buffer)) T(CASTLE_MOVE(*other.get()));
        other.~soo_buffer(); // Clean up the moved-from object
    }

    soo_buffer& operator=(soo_buffer&& other) CASTLE_NOEXCEPT
    {
        if (this != &other)
        {
            this->~soo_buffer(); // Clean up existing resource
            ::new (static_cast<void*>(inline_buffer)) T(CASTLE_MOVE(*other.get()));
            other.~soo_buffer(); // Clean up the moved-from object
        }
        return *this;
    }

    ~soo_buffer()
    {
        castle::memory::launder(reinterpret_cast<T*>(inline_buffer))->~T();
    }

    T* get() CASTLE_NOEXCEPT
    {
        return castle::memory::launder(reinterpret_cast<T*>(inline_buffer));
    }

    CASTLE_CONST T* get() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return castle::memory::launder(reinterpret_cast<CASTLE_CONST T*>(inline_buffer));
    }

private:
    union alignas(alignof(T) > alignof(T*) ? alignof(T) : alignof(T*))
    {
        char inline_buffer[StackSize];
    };
};

} // namespace memory
} // namespace castle

#endif // CASTLE_MEMORY_SOO_BUFFER_H
