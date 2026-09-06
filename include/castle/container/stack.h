#ifndef CASTLE_CONTAINER_STACK_H
#define CASTLE_CONTAINER_STACK_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"
#include "castle/core/types.h"
#include "castle/utility/move.h"
#include "castle/container/array.h"

namespace castle
{
namespace container
{

// Fixed-capacity LIFO stack buffer. head_ always denotes the next write slot.
template <typename T, size_type N>
class stack_buffer
{
    static_assert(N > 0U, "stack_buffer capacity must be non-zero");
    static_assert(meta::is_trivially_copyable<T>::value,
                  "stack_buffer<T,N> requires T to be trivially copyable");
    static_assert(meta::is_trivially_destructible<T>::value,
                  "stack_buffer<T,N> requires T to be trivially destructible");

public:
    using value_type      = T;
    using size_type       = castle::size_type;
    using difference_type = castle::difference_type;
    using reference       = T&;
    using const_reference = CASTLE_CONST T&;

    stack_buffer() CASTLE_NOEXCEPT CASTLE_DEFAULT;

    stack_buffer(CASTLE_CONST stack_buffer&)            CASTLE_DELETE;
    stack_buffer& operator=(CASTLE_CONST stack_buffer&) CASTLE_DELETE;
    stack_buffer(stack_buffer&&)                        CASTLE_DELETE;
    stack_buffer& operator=(stack_buffer&&)             CASTLE_DELETE;

    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT { return N; }

    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return size_; }
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == 0U; }
    bool full() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == N; }
    size_type available() CASTLE_CONST CASTLE_NOEXCEPT { return N - size_; }

    void clear() CASTLE_NOEXCEPT
    {
        head_ = 0U;
        size_ = 0U;
    }

    bool push(CASTLE_CONST value_type& value) CASTLE_NOEXCEPT
    {
        if (full())
        {
            return false;
        }
        data_[head_] = value;
        head_ = advance(head_);
        ++size_;
        return true;
    }

    bool push(value_type&& value) CASTLE_NOEXCEPT
    {
        if (full())
        {
            return false;
        }
        data_[head_] = castle::move(value);
        head_ = advance(head_);
        ++size_;
        return true;
    }

    // Appends value. When full, the current newest item is replaced.
    // The logical size remains N.
    bool force_push(CASTLE_CONST value_type& value) CASTLE_NOEXCEPT
    {
        CASTLE_CONST bool evicted = full();
        if (evicted)
        {
            head_ = retreat(head_);
            data_[head_] = value;
            head_ = advance(head_);
        }
        else
        {
            data_[head_] = value;
            head_ = advance(head_);
            ++size_;
        }
        return evicted;
    }

    bool force_push(value_type&& value) CASTLE_NOEXCEPT
    {
        CASTLE_CONST bool evicted = full();
        if (evicted)
        {
            head_ = retreat(head_);
            data_[head_] = castle::move(value);
            head_ = advance(head_);
        }
        else
        {
            data_[head_] = castle::move(value);
            head_ = advance(head_);
            ++size_;
        }
        return evicted;
    }

    bool pop(value_type& out) CASTLE_NOEXCEPT
    {
        if (empty())
        {
            return false;
        }
        head_ = retreat(head_);
        out = data_[head_];
        --size_;
        return true;
    }

    bool pop() CASTLE_NOEXCEPT
    {
        if (empty())
        {
            return false;
        }
        head_ = retreat(head_);
        --size_;
        return true;
    }

    // index 0 is the oldest element; size()-1 is the newest element.
    bool peek(size_type index, value_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (index >= size_)
        {
            return false;
        }
        out = data_[wrap(head_ + N - size_ + index)];
        return true;
    }

    // Returns the newest element. Precondition: !empty().
    reference top() CASTLE_NOEXCEPT { return data_[wrap(head_ + N - 1U)]; }
    const_reference top() CASTLE_CONST CASTLE_NOEXCEPT { return data_[wrap(head_ + N - 1U)]; }

    size_type push_bulk(CASTLE_CONST value_type* src, size_type max) CASTLE_NOEXCEPT
    {
        if (src == nullptr)
        {
            return 0U;
        }
        size_type written = 0U;
        while ((written < max) && !full())
        {
            data_[head_] = src[written];
            head_ = advance(head_);
            ++size_;
            ++written;
        }
        return written;
    }

    // Pops in LIFO order: newest element first.
    size_type pop_bulk(value_type* dst, size_type max) CASTLE_NOEXCEPT
    {
        if (dst == nullptr)
        {
            return 0U;
        }
        size_type read = 0U;
        while ((read < max) && !empty())
        {
            head_ = retreat(head_);
            dst[read] = data_[head_];
            --size_;
            ++read;
        }
        return read;
    }

private:
    static CASTLE_CONSTEXPR size_type wrap(size_type index) CASTLE_NOEXCEPT
    {
        return meta::is_power_of_two<N>::value
                   ? (index & (N - 1U))
                   : (index % N);
    }

    static CASTLE_CONSTEXPR size_type advance(size_type index) CASTLE_NOEXCEPT
    {
        return wrap(index + 1U);
    }

    static CASTLE_CONSTEXPR size_type retreat(size_type index) CASTLE_NOEXCEPT
    {
        return wrap(index + N - 1U);
    }

    container::array<value_type, N> data_{};
    size_type head_ = 0U;
    size_type size_ = 0U;
};

} // namespace container
} // namespace castle

#endif // CASTLE_CONTAINER_STACK_H
