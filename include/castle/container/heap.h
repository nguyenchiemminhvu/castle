#ifndef CASTLE_CONTAINER_HEAP_H
#define CASTLE_CONTAINER_HEAP_H

#include "castle/core/compiler.h"
#include "castle/core/types.h"
#include "castle/core/traits.h"
#include "castle/error/status.h"
#include "castle/memory/object.h"
#include "castle/memory/construct.h"
#include "castle/memory/destroy.h"
#include "castle/memory/static_storage.h"
#include "castle/utility/compare.h"
#include "castle/utility/forward.h"
#include "castle/utility/move.h"

namespace castle
{
namespace container
{

template <typename T, size_type N, typename Compare>
class basic_heap
{
    static_assert(N > 0U, "basic_heap requires a positive capacity");
    static_assert(meta::is_destructible<T>::value,
                  "basic_heap<T,N> requires a destructible T");

public:
    using value_type = T;
    using size_type = castle::size_type;
    using difference_type = castle::difference_type;
    using reference = T&;
    using const_reference = CASTLE_CONST T&;
    using pointer = T*;
    using const_pointer = CASTLE_CONST T*;
    using iterator = pointer;
    using const_iterator = const_pointer;

    basic_heap() CASTLE_NOEXCEPT : size_(0U), compare_() {}
    explicit basic_heap(CASTLE_CONST Compare& compare) CASTLE_NOEXCEPT : size_(0U), compare_(compare) {}

    ~basic_heap() CASTLE_NOEXCEPT { clear(); }

    basic_heap(CASTLE_CONST basic_heap&) CASTLE_DELETE;
    basic_heap& operator=(CASTLE_CONST basic_heap&) CASTLE_DELETE;
    basic_heap(basic_heap&&) CASTLE_DELETE;
    basic_heap& operator=(basic_heap&&) CASTLE_DELETE;

    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT { return N; }
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return size_; }
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == 0U; }
    bool full() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == N; }
    size_type available() CASTLE_CONST CASTLE_NOEXCEPT { return N - size_; }

    reference top() CASTLE_NOEXCEPT { return *pointer_at(0U); }
    const_reference top() CASTLE_CONST CASTLE_NOEXCEPT { return *pointer_at(0U); }

    status push(CASTLE_CONST T& value) CASTLE_NOEXCEPT
    {
        if (full())
        {
            return status::full;
        }

        memory::construct_at<T>(storage_.address(size_), value);
        ++size_;
        sift_up(size_ - 1U);
        return status::ok;
    }

    status push(T&& value) CASTLE_NOEXCEPT
    {
        if (full())
        {
            return status::full;
        }

        memory::construct_at<T>(storage_.address(size_), CASTLE_MOVE(value));
        ++size_;
        sift_up(size_ - 1U);
        return status::ok;
    }

    template <typename... Args>
    status emplace(Args&&... args) CASTLE_NOEXCEPT
    {
        if (full())
        {
            return status::full;
        }

        memory::construct_at<T>(storage_.address(size_), CASTLE_FORWARD<Args>(args)...);
        ++size_;
        sift_up(size_ - 1U);
        return status::ok;
    }

    status pop(T& out) CASTLE_NOEXCEPT
    {
        if (empty())
        {
            return status::empty;
        }

        out = CASTLE_MOVE(*pointer_at(0U));
        return pop();
    }

    status pop() CASTLE_NOEXCEPT
    {
        if (empty())
        {
            return status::empty;
        }

        CASTLE_CONST size_type last = size_ - 1U;
        if (last != 0U)
        {
            *pointer_at(0U) = CASTLE_MOVE(*pointer_at(last));
        }
        memory::destroy_at(pointer_at(last));
        --size_;
        if (size_ > 0U)
        {
            sift_down(0U);
        }
        return status::ok;
    }

    status remove_at(size_type index) CASTLE_NOEXCEPT
    {
        if (index >= size_)
        {
            return status::out_of_range;
        }

        CASTLE_CONST size_type last = size_ - 1U;
        if (index != last)
        {
            *pointer_at(index) = CASTLE_MOVE(*pointer_at(last));
        }
        memory::destroy_at(pointer_at(last));
        --size_;
        if (index < size_)
        {
            if ((index > 0U) && higher_priority(index, parent(index)))
            {
                sift_up(index);
            }
            else
            {
                sift_down(index);
            }
        }
        return status::ok;
    }

    status replace_top(T& value) CASTLE_NOEXCEPT
    {
        if (empty())
        {
            return status::empty;
        }
        *pointer_at(0U) = CASTLE_MOVE(value);
        sift_down(0U);
        return status::ok;
    }

    status replace_top(CASTLE_CONST T& value) CASTLE_NOEXCEPT
    {
        if (empty())
        {
            return status::empty;
        }
        *pointer_at(0U) = value;
        sift_down(0U);
        return status::ok;
    }

    void clear() CASTLE_NOEXCEPT
    {
        for (size_type i = size_; i > 0U; --i)
        {
            memory::destroy_at(pointer_at(i - 1U));
        }
        size_ = 0U;
    }

    iterator begin() CASTLE_NOEXCEPT { return size_ == 0U ? nullptr : pointer_at(0U); }
    const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == 0U ? nullptr : pointer_at(0U); }
    iterator end() CASTLE_NOEXCEPT { return size_ == 0U ? nullptr : pointer_at(size_); }
    const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == 0U ? nullptr : pointer_at(size_); }

private:
    pointer pointer_at(size_type index) CASTLE_NOEXCEPT
    {
        return memory::object_from_address<T>(storage_.address(index));
    }

    CASTLE_CONST T* pointer_at(size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return memory::object_from_address<CASTLE_CONST T>(storage_.address(index));
    }

    static CASTLE_CONSTEXPR size_type parent(size_type index) CASTLE_NOEXCEPT
    {
        return (index - 1U) / 2U;
    }

    static CASTLE_CONSTEXPR size_type left(size_type index) CASTLE_NOEXCEPT
    {
        return index * 2U + 1U;
    }

    bool higher_priority(size_type lhs, size_type rhs) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return compare_(*pointer_at(lhs), *pointer_at(rhs));
    }

    void sift_up(size_type index) CASTLE_NOEXCEPT
    {
        while (index > 0U)
        {
            CASTLE_CONST size_type p = parent(index);
            if (!higher_priority(index, p))
            {
                break;
            }
            swap_values(index, p);
            index = p;
        }
    }

    void sift_down(size_type index) CASTLE_NOEXCEPT
    {
        for (;;)
        {
            CASTLE_CONST size_type child = left(index);
            if (child >= size_)
            {
                break;
            }
            size_type selected = child;
            CASTLE_CONST size_type right = child + 1U;
            if ((right < size_) && higher_priority(right, child))
            {
                selected = right;
            }
            if (!higher_priority(selected, index))
            {
                break;
            }
            swap_values(selected, index);
            index = selected;
        }
    }

    void swap_values(size_type lhs, size_type rhs) CASTLE_NOEXCEPT
    {
        T temp(CASTLE_MOVE(*pointer_at(lhs)));
        *pointer_at(lhs) = CASTLE_MOVE(*pointer_at(rhs));
        *pointer_at(rhs) = CASTLE_MOVE(temp);
    }

    size_type size_;
    Compare compare_;
    memory::static_storage<T, N> storage_;
};

template <typename T, size_type N, typename Compare = castle::greater<T>>
using max_heap = basic_heap<T, N, Compare>;

template <typename T, size_type N, typename Compare = castle::less<T>>
using min_heap = basic_heap<T, N, Compare>;

} // namespace container
} // namespace castle

#endif // CASTLE_CONTAINER_HEAP_H
