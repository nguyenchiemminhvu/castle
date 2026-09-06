#ifndef CASTLE_CONTAINER_VECTOR_H
#define CASTLE_CONTAINER_VECTOR_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"
#include "castle/core/types.h"
#include "castle/error/status.h"
#include "castle/iterator/reverse_iterator.h"
#include "castle/memory/object.h"
#include "castle/memory/construct.h"
#include "castle/memory/destroy.h"
#include "castle/memory/object.h"
#include "castle/memory/static_storage.h"
#include "castle/utility/forward.h"
#include "castle/utility/move.h"
#include <stddef.h>

namespace castle
{
namespace container
{

// Fixed-capacity vector. Storage is embedded in the vector object and only the
// first size() slots contain live T objects.
template <typename T, size_t N>
class vector
{
    static_assert(N > 0U, "vector requires a positive capacity");
    static_assert(meta::is_destructible<T>::value,
                  "vector<T,N> requires a destructible T");
public:
    using value_type = T;
    using size_type = castle::size_type;
    using difference_type = castle::difference_type;
    using reference = T&;
    using const_reference = CASTLE_CONST T&;
    using pointer = T*;
    using const_pointer = CASTLE_CONST T*;
    using iterator = T*;
    using const_iterator = CASTLE_CONST T*;
    using reverse_iterator = castle::reverse_iterator<iterator>;
    using const_reverse_iterator = castle::reverse_iterator<const_iterator>;
    static CASTLE_CONSTEXPR size_type static_capacity = N;

    vector() CASTLE_NOEXCEPT : size_(0U) {}

    ~vector() CASTLE_NOEXCEPT
    {
        clear();
    }

    vector(CASTLE_CONST vector& other) : size_(0U)
    {
        static_assert(meta::is_copy_constructible<T>::value,
                      "vector copy construction requires a copy-constructible T");
        for (size_type i = 0U; i < other.size_; ++i)
        {
            // The increment follows successful construction. If T construction
            // throws, the partially constructed vector owns exactly size_ live T.
            memory::construct_at<T>(storage_.address(i), other[i]);
            ++size_;
        }
    }

    vector& operator=(CASTLE_CONST vector& other)
    {
        if (this != &other)
        {
            static_assert(meta::is_copy_constructible<T>::value,
                          "vector copy assignment requires a copy-constructible T");
            clear();
            for (size_type i = 0U; i < other.size_; ++i)
            {
                memory::construct_at<T>(storage_.address(i), other[i]);
                ++size_;
            }
        }
        return *this;
    }

    vector(vector&& other) CASTLE_NOEXCEPT(meta::is_nothrow_move_constructible<T>::value)
        : size_(0U)
    {
        static_assert(meta::is_move_constructible<T>::value,
                      "vector move construction requires a move-constructible T");

        for (size_type i = 0U; i < other.size_; ++i)
        {
            memory::construct_at<T>(storage_.address(i), CASTLE_MOVE(other[i]));
            ++size_;
        }
        other.clear();
    }

    vector& operator=(vector&& other) CASTLE_NOEXCEPT(meta::is_nothrow_move_constructible<T>::value)
    {
        if (this != &other)
        {
            static_assert(meta::is_move_constructible<T>::value,
                          "vector move assignment requires a move-constructible T");
            clear();
            for (size_type i = 0U; i < other.size_; ++i)
            {
                memory::construct_at<T>(storage_.address(i), CASTLE_MOVE(other[i]));
                ++size_;
            }
            other.clear();
        }
        return *this;
    }

    CASTLE_CONSTEXPR size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return size_; }
    CASTLE_CONSTEXPR size_type capacity() CASTLE_CONST CASTLE_NOEXCEPT { return N; }
    CASTLE_CONSTEXPR bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == 0U; }
    CASTLE_CONSTEXPR bool full() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == N; }

    // The empty iterator is a stable pointer into the vector's storage instead
    // of nullptr. This keeps begin()==end() well-defined for an empty vector and
    // avoids pointer subtraction between null pointers.
    iterator begin() CASTLE_NOEXCEPT
    {
        return pointer_at(0U);
    }

    const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return pointer_at(0U);
    }

    const_iterator cbegin() CASTLE_CONST CASTLE_NOEXCEPT { return begin(); }

    iterator end() CASTLE_NOEXCEPT
    {
        return begin() + size_;
    }

    const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return begin() + size_;
    }

    const_iterator cend() CASTLE_CONST CASTLE_NOEXCEPT { return end(); }
    reverse_iterator rbegin() CASTLE_NOEXCEPT { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() CASTLE_CONST CASTLE_NOEXCEPT { return const_reverse_iterator(end()); }
    const_reverse_iterator crbegin() CASTLE_CONST CASTLE_NOEXCEPT { return const_reverse_iterator(end()); }
    reverse_iterator rend() CASTLE_NOEXCEPT { return reverse_iterator(begin()); }
    const_reverse_iterator rend() CASTLE_CONST CASTLE_NOEXCEPT { return const_reverse_iterator(begin()); }
    const_reverse_iterator crend() CASTLE_CONST CASTLE_NOEXCEPT { return const_reverse_iterator(begin()); }

    reference operator[](size_type index) CASTLE_NOEXCEPT
    {
        return *pointer_at(index);
    }

    const_reference operator[](size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return *pointer_at(index);
    }

    reference front() CASTLE_NOEXCEPT { return *begin(); }
    const_reference front() CASTLE_CONST CASTLE_NOEXCEPT { return *begin(); }
    reference back() CASTLE_NOEXCEPT { return *(end() - 1); }
    const_reference back() CASTLE_CONST CASTLE_NOEXCEPT { return *(end() - 1); }
    pointer data() CASTLE_NOEXCEPT { return pointer_at(0U); }
    const_pointer data() CASTLE_CONST CASTLE_NOEXCEPT { return pointer_at(0U); }

    template <typename... Args>
    status emplace_back(Args&&... args)
        CASTLE_NOEXCEPT(meta::is_nothrow_constructible<T, Args&&...>::value)
    {
        if (full())
        {
            return status::full;
        }
        memory::construct_at<T>(storage_.address(size_), CASTLE_FORWARD<Args>(args)...);
        ++size_;
        return status::ok;
    }

    status push_back(CASTLE_CONST T& value)
        CASTLE_NOEXCEPT(meta::is_nothrow_constructible<T, CASTLE_CONST T&>::value)
    {
        return emplace_back(value);
    }

    status push_back(T&& value)
        CASTLE_NOEXCEPT(meta::is_nothrow_constructible<T, T&&>::value)
    {
        return emplace_back(CASTLE_MOVE(value));
    }

    status pop_back() CASTLE_NOEXCEPT
    {
        if (empty())
        {
            return status::empty;
        }

        --size_;
        memory::destroy_at(pointer_at(size_));
        return status::ok;
    }

    void clear() CASTLE_NOEXCEPT
    {
        memory::destroy_n(pointer_at(0U), size_);
        size_ = 0U;
    }

private:
    pointer pointer_at(size_type index) CASTLE_NOEXCEPT
    {
        return memory::object_from_address<T>(storage_.address(index));
    }

    const_pointer pointer_at(size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return memory::object_from_address<CASTLE_CONST T>(storage_.address(index));
    }

    size_type size_;
    memory::static_storage<T, N> storage_;
};

} // namespace container
} // namespace castle
#endif // CASTLE_CONTAINER_VECTOR_H
