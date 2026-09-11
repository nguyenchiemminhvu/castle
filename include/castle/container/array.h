#ifndef CASTLE_CONTAINER_ARRAY_H
#define CASTLE_CONTAINER_ARRAY_H

#include "castle/core/compiler.h"
#include "castle/core/types.h"
#include "castle/iterator/reverse_iterator.h"
#include "castle/core/traits.h"
#include "castle/utility/forward.h"

#include <stddef.h>

namespace castle
{
namespace container
{

// Fixed-size contiguous aggregate. All N elements are live objects.
template <typename T, size_t N>
class array
{
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

    static CASTLE_CONSTEXPR size_type static_size = N;

    CASTLE_CONSTEXPR array() CASTLE_DEFAULT;

    template <typename... Args,
              meta::enable_if_t<(sizeof...(Args) == N) &&
                                meta::conjunction<meta::is_constructible<T, Args&&>...>::value, int> = 0>
    CASTLE_CONSTEXPR array(Args&&... args)
        : data_{CASTLE_FORWARD<Args>(args)...}
    {
    }

    CASTLE_CONSTEXPR size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return N; }
    CASTLE_CONSTEXPR size_type capacity() CASTLE_CONST CASTLE_NOEXCEPT { return N; }
    CASTLE_CONSTEXPR bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return N == 0U; }
    CASTLE_CONSTEXPR bool full() CASTLE_CONST CASTLE_NOEXCEPT { return N == 0U || N == size(); }

    CASTLE_CONSTEXPR iterator begin() CASTLE_NOEXCEPT { return data_; }
    CASTLE_CONSTEXPR const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }
    CASTLE_CONSTEXPR const_iterator cbegin() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }

    CASTLE_CONSTEXPR iterator end() CASTLE_NOEXCEPT { return data_ + N; }
    CASTLE_CONSTEXPR const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return data_ + N; }
    CASTLE_CONSTEXPR const_iterator cend() CASTLE_CONST CASTLE_NOEXCEPT { return data_ + N; }

    CASTLE_CONSTEXPR reverse_iterator rbegin() CASTLE_NOEXCEPT { return reverse_iterator(end()); }
    CASTLE_CONSTEXPR const_reverse_iterator rbegin() CASTLE_CONST CASTLE_NOEXCEPT { return const_reverse_iterator(end()); }
    CASTLE_CONSTEXPR const_reverse_iterator crbegin() CASTLE_CONST CASTLE_NOEXCEPT { return const_reverse_iterator(end()); }

    CASTLE_CONSTEXPR reverse_iterator rend() CASTLE_NOEXCEPT { return reverse_iterator(begin()); }
    CASTLE_CONSTEXPR const_reverse_iterator rend() CASTLE_CONST CASTLE_NOEXCEPT { return const_reverse_iterator(begin()); }
    CASTLE_CONSTEXPR const_reverse_iterator crend() CASTLE_CONST CASTLE_NOEXCEPT { return const_reverse_iterator(begin()); }

    CASTLE_CONSTEXPR reference operator[](size_type index) CASTLE_NOEXCEPT { return data_[index]; }
    CASTLE_CONSTEXPR const_reference operator[](size_type index) CASTLE_CONST CASTLE_NOEXCEPT { return data_[index]; }

    CASTLE_CONSTEXPR reference front() CASTLE_NOEXCEPT { return data_[0U]; }
    CASTLE_CONSTEXPR const_reference front() CASTLE_CONST CASTLE_NOEXCEPT { return data_[0U]; }
    CASTLE_CONSTEXPR reference back() CASTLE_NOEXCEPT { return data_[N - 1U]; }
    CASTLE_CONSTEXPR const_reference back() CASTLE_CONST CASTLE_NOEXCEPT { return data_[N - 1U]; }

    CASTLE_CONSTEXPR pointer data() CASTLE_NOEXCEPT { return data_; }
    CASTLE_CONSTEXPR const_pointer data() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }

private:
    T data_[N];
};

// Zero-sized specialization avoids the non-standard T[0] extension and keeps
// array<T, 0> a valid type. Element access is intentionally unavailable.
template <typename T>
class array<T, 0U>
{
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

    static CASTLE_CONSTEXPR size_type static_size = 0U;

    CASTLE_CONSTEXPR size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return 0U; }
    CASTLE_CONSTEXPR size_type capacity() CASTLE_CONST CASTLE_NOEXCEPT { return 0U; }
    CASTLE_CONSTEXPR bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return true; }
    CASTLE_CONSTEXPR bool full() CASTLE_CONST CASTLE_NOEXCEPT { return true; }

    CASTLE_CONSTEXPR iterator begin() CASTLE_NOEXCEPT { return nullptr; }
    CASTLE_CONSTEXPR const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return nullptr; }
    CASTLE_CONSTEXPR const_iterator cbegin() CASTLE_CONST CASTLE_NOEXCEPT { return nullptr; }
    CASTLE_CONSTEXPR iterator end() CASTLE_NOEXCEPT { return nullptr; }
    CASTLE_CONSTEXPR const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return nullptr; }
    CASTLE_CONSTEXPR const_iterator cend() CASTLE_CONST CASTLE_NOEXCEPT { return nullptr; }
    CASTLE_CONSTEXPR reverse_iterator rbegin() CASTLE_NOEXCEPT { return reverse_iterator(nullptr); }
    CASTLE_CONSTEXPR const_reverse_iterator rbegin() CASTLE_CONST CASTLE_NOEXCEPT { return const_reverse_iterator(nullptr); }
    CASTLE_CONSTEXPR const_reverse_iterator crbegin() CASTLE_CONST CASTLE_NOEXCEPT { return const_reverse_iterator(nullptr); }
    CASTLE_CONSTEXPR reverse_iterator rend() CASTLE_NOEXCEPT { return reverse_iterator(nullptr); }
    CASTLE_CONSTEXPR const_reverse_iterator rend() CASTLE_CONST CASTLE_NOEXCEPT { return const_reverse_iterator(nullptr); }
    CASTLE_CONSTEXPR const_reverse_iterator crend() CASTLE_CONST CASTLE_NOEXCEPT { return const_reverse_iterator(nullptr); }
    CASTLE_CONSTEXPR pointer data() CASTLE_NOEXCEPT { return nullptr; }
    CASTLE_CONSTEXPR const_pointer data() CASTLE_CONST CASTLE_NOEXCEPT { return nullptr; }
};

} // namespace container
} // namespace castle

#endif // CASTLE_CONTAINER_ARRAY_H
