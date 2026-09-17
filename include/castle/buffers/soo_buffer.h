#pragma once

#include <cstdint>
#include <new>
#include <utility>
#include <type_traits>

namespace castle
{
namespace buffers
{
template <typename T, std::size_t StackSize = 32, bool HeapAllowed = true>
class soo_buffer
{
public:
    template <typename Arg, typename = std::enable_if_t<HeapAllowed || sizeof(T) <= StackSize, void>>
    soo_buffer(Arg&& arg)
        : is_heap_allocated(false)
    {
        if constexpr (sizeof(T) <= StackSize)
        {
            ::new (static_cast<void*>(inline_buffer)) Arg(std::forward<Arg>(arg));
        }
        else
        {
            warn_if_heap_allocation();
            heap_buffer = new T(std::forward<Arg>(arg));
            is_heap_allocated = true;
        }
    }

    soo_buffer(const soo_buffer&) = delete;
    soo_buffer& operator=(const soo_buffer&) = delete;

    soo_buffer(soo_buffer&& other) noexcept
        : is_heap_allocated(other.is_heap_allocated)
    {
        this->~soo_buffer(); // Clean up existing resource
        if (other.is_heap_allocated)
        {
            heap_buffer = other.heap_buffer;
            other.heap_buffer = nullptr;
            other.is_heap_allocated = false; // Steal pointer ownership
        }
        else
        {
            ::new (static_cast<void*>(inline_buffer)) T(std::move(*other.get()));
        }
    }

    soo_buffer& operator=(soo_buffer&& other) noexcept
    {
        if (this != &other)
        {
            this->~soo_buffer(); // Clean up existing resource
            is_heap_allocated = other.is_heap_allocated;
            if (other.is_heap_allocated)
            {
                heap_buffer = other.heap_buffer;
                other.heap_buffer = nullptr;
                other.is_heap_allocated = false;
            }
            else
            {
                ::new (static_cast<void*>(inline_buffer)) T(std::move(*other.get()));
            }
        }
        return *this;
    }

    ~soo_buffer()
    {
        if (is_heap_allocated)
        {
            delete heap_buffer;
        }
        else
        {
            std::launder(reinterpret_cast<T*>(inline_buffer))->~T();
        }
    }

    T* get() noexcept
    {
        if (is_heap_allocated)
        {
            return heap_buffer;
        }
        else
        {
            return std::launder(reinterpret_cast<T*>(inline_buffer));
        }
    }

    const T* get() const noexcept
    {
        if (is_heap_allocated)
        {
            return heap_buffer;
        }
        else
        {
            return std::launder(reinterpret_cast<const T*>(inline_buffer));
        }
    }

    bool is_heap() const noexcept { return is_heap_allocated; }

private:
    template <typename U = T>
    [[deprecated("soo_buffer: Allocating on heap because size of T exceeds StackSize")]]
    void warn_if_heap_allocation() {}
    union alignas(alignof(T) > alignof(T*) ? alignof(T) : alignof(T*))
    {
        char inline_buffer[StackSize];
        T* heap_buffer;
    };
    bool is_heap_allocated;
};

} // namespace buffers
} // namespace castle