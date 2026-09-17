// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file array.hpp
 * @brief Fixed-size contiguous container with inline storage.
 *
 * Use `array<T, N>` when the number of elements is known at compile time and
 * all `N` objects should live directly inside the container. The container
 * never allocates, `size()` and `capacity()` are both fixed at `N`, and
 * element access plus iterator operations are O(1). Because every slot is
 * always part of the array, `full()` always reports `true`.
 *
 * @code
 * castle::container::array<int, 3U> values{1, 2, 3};
 * values[1] = 20;
 * int tail = values.back();
 * @endcode
 */
#ifndef CASTLE_CONTAINER_ARRAY_HPP
#define CASTLE_CONTAINER_ARRAY_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/iterator/reverse_iterator.hpp"
#include "castle/core/traits.hpp"
#include "castle/utility/forward.hpp"

#include "castle/core/error_handler.hpp"
#include "castle/container/initializer_list.hpp"

#include <stddef.h>

namespace castle
{
namespace container
{

/**
 * @brief Fixed-size contiguous container whose storage is embedded in the object.
 * @tparam T Element type.
 * @tparam N Number of elements stored inline.
 * @note All `N` elements are always present. `size()` and `capacity()` both
 * equal `N`, and `full()` always returns `true`.
 * @note Iterators, pointers, and references remain valid until the array
 * object itself is destroyed.
 * @warning `front()`, `back()`, and `operator[]` require a valid index and are
 * unavailable only in the `array<T, 0U>` specialization.
 */
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

    /**
     * @brief Compile-time element count.
     */
    static CASTLE_CONSTEXPR size_type static_size = N;

    /**
     * @brief Default-constructs all `N` elements.
     * @note Complexity: O(N) through element default construction.
     */
    CASTLE_CONSTEXPR array() CASTLE_DEFAULT;

    /**
     * @brief Constructs the array from exactly `N` forwarded arguments.
     * @tparam Args Argument types used to initialize the elements.
     * @param args Element constructor arguments supplied in order.
     * @note Complexity: O(N).
     * @warning The overload participates only when exactly `N` arguments are
     * provided and each element is constructible from its corresponding argument.
     */
    template <typename... Args,
              meta::enable_if_t<(sizeof...(Args) == N) &&
                                meta::conjunction<meta::is_constructible<T, Args&&>...>::value, int> = 0>
    CASTLE_CONSTEXPR array(Args&&... args)
        : data_{CASTLE_FORWARD<Args>(args)...}
    {
    }

    /**
     * @brief Constructs the array from an initializer list.
     * @param init Initializer list supplying exactly `N` elements.
     * @note Complexity: O(N).
     * @warning `init.size()` must equal `N`; the constructor reports a violated
     * precondition with `CASTLE_ASSERT`.
     */
    CASTLE_CONSTEXPR array(initializer_list<T> init)
    {
        CASTLE_ASSERT(init.size() == N, "Initializer list size must match array size"); // LCOV_EXCL_BR_LINE

        size_type i = 0U;
        for (const auto& elem : init)
        {
            data_[i++] = elem;
        }
    }

    /**
     * @brief Returns the number of elements.
     * @return Always `N`.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return N; }

    /**
     * @brief Returns the capacity.
     * @return Always `N`.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR size_type capacity() CASTLE_CONST CASTLE_NOEXCEPT { return N; }

    /**
     * @brief Reports whether the array has zero elements.
     * @return `true` only when `N == 0`.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return N == 0U; }

    /**
     * @brief Reports whether the array is full.
     * @return Always `true` because the array has fixed size.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR bool full() CASTLE_CONST CASTLE_NOEXCEPT { return N == 0U || N == size(); }

    /**
     * @brief Returns a mutable iterator to the first element.
     * @return Iterator to the first element.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR iterator begin() CASTLE_NOEXCEPT { return data_; }

    /**
     * @brief Returns a const iterator to the first element.
     * @return Const iterator to the first element.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }

    /**
     * @brief Returns a const iterator to the first element.
     * @return Const iterator to the first element.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR const_iterator cbegin() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }

    /**
     * @brief Returns a mutable iterator past the last element.
     * @return Iterator one past the final element.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR iterator end() CASTLE_NOEXCEPT { return data_ + N; }

    /**
     * @brief Returns a const iterator past the last element.
     * @return Const iterator one past the final element.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return data_ + N; }

    /**
     * @brief Returns a const iterator past the last element.
     * @return Const iterator one past the final element.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR const_iterator cend() CASTLE_CONST CASTLE_NOEXCEPT { return data_ + N; }

    /**
     * @brief Returns a mutable reverse iterator to the last element.
     * @return Reverse iterator to the last element.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR reverse_iterator rbegin() CASTLE_NOEXCEPT { return reverse_iterator(end()); }

    /**
     * @brief Returns a const reverse iterator to the last element.
     * @return Const reverse iterator to the last element.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR const_reverse_iterator rbegin() CASTLE_CONST CASTLE_NOEXCEPT { return const_reverse_iterator(end()); }

    /**
     * @brief Returns a const reverse iterator to the last element.
     * @return Const reverse iterator to the last element.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR const_reverse_iterator crbegin() CASTLE_CONST CASTLE_NOEXCEPT { return const_reverse_iterator(end()); }

    /**
     * @brief Returns a mutable reverse-end iterator.
     * @return Reverse iterator one element before the first element.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR reverse_iterator rend() CASTLE_NOEXCEPT { return reverse_iterator(begin()); }

    /**
     * @brief Returns a const reverse-end iterator.
     * @return Const reverse iterator one element before the first element.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR const_reverse_iterator rend() CASTLE_CONST CASTLE_NOEXCEPT { return const_reverse_iterator(begin()); }

    /**
     * @brief Returns a const reverse-end iterator.
     * @return Const reverse iterator one element before the first element.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR const_reverse_iterator crend() CASTLE_CONST CASTLE_NOEXCEPT { return const_reverse_iterator(begin()); }

    /**
     * @brief Returns the element at `index`.
     * @param index Zero-based element index.
     * @return Mutable reference to the indexed element.
     * @note Complexity: O(1).
     * @warning `index` must be less than `N`.
     */
    CASTLE_CONSTEXPR reference operator[](size_type index) CASTLE_NOEXCEPT { return data_[index]; }

    /**
     * @brief Returns the element at `index`.
     * @param index Zero-based element index.
     * @return Const reference to the indexed element.
     * @note Complexity: O(1).
     * @warning `index` must be less than `N`.
     */
    CASTLE_CONSTEXPR const_reference operator[](size_type index) CASTLE_CONST CASTLE_NOEXCEPT { return data_[index]; }

    /**
     * @brief Returns the first element.
     * @return Mutable reference to the first element.
     * @note Complexity: O(1).
     * @warning `N` must be greater than `0`.
     */
    CASTLE_CONSTEXPR reference front() CASTLE_NOEXCEPT { return data_[0U]; }

    /**
     * @brief Returns the first element.
     * @return Const reference to the first element.
     * @note Complexity: O(1).
     * @warning `N` must be greater than `0`.
     */
    CASTLE_CONSTEXPR const_reference front() CASTLE_CONST CASTLE_NOEXCEPT { return data_[0U]; }

    /**
     * @brief Returns the last element.
     * @return Mutable reference to the last element.
     * @note Complexity: O(1).
     * @warning `N` must be greater than `0`.
     */
    CASTLE_CONSTEXPR reference back() CASTLE_NOEXCEPT { return data_[N - 1U]; }

    /**
     * @brief Returns the last element.
     * @return Const reference to the last element.
     * @note Complexity: O(1).
     * @warning `N` must be greater than `0`.
     */
    CASTLE_CONSTEXPR const_reference back() CASTLE_CONST CASTLE_NOEXCEPT { return data_[N - 1U]; }

    /**
     * @brief Returns a pointer to the first element.
     * @return Mutable pointer to the first element.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR pointer data() CASTLE_NOEXCEPT { return data_; }

    /**
     * @brief Returns a pointer to the first element.
     * @return Const pointer to the first element.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR const_pointer data() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }

private:
    T data_[N];
};

/**
 * @brief Zero-sized array specialization.
 * @tparam T Element type.
 * @note The specialization is valid and allocation-free, but exposes only
 * size/capacity queries, iterators, and `data()`. Element accessors are absent.
 * @note All operations are O(1).
 */
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

    /**
     * @brief Compile-time element count.
     */
    static CASTLE_CONSTEXPR size_type static_size = 0U;

    /**
     * @brief Returns the number of elements.
     * @return Always `0`.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return 0U; }

    /**
     * @brief Returns the capacity.
     * @return Always `0`.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR size_type capacity() CASTLE_CONST CASTLE_NOEXCEPT { return 0U; }

    /**
     * @brief Reports that the array is empty.
     * @return Always `true`.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return true; }

    /**
     * @brief Reports that the array is full.
     * @return Always `true`.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR bool full() CASTLE_CONST CASTLE_NOEXCEPT { return true; }

    /**
     * @brief Returns a mutable iterator to the first element.
     * @return Always `nullptr`.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR iterator begin() CASTLE_NOEXCEPT { return nullptr; }

    /**
     * @brief Returns a const iterator to the first element.
     * @return Always `nullptr`.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return nullptr; }

    /**
     * @brief Returns a const iterator to the first element.
     * @return Always `nullptr`.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR const_iterator cbegin() CASTLE_CONST CASTLE_NOEXCEPT { return nullptr; }

    /**
     * @brief Returns a mutable end iterator.
     * @return Always `nullptr`.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR iterator end() CASTLE_NOEXCEPT { return nullptr; }

    /**
     * @brief Returns a const end iterator.
     * @return Always `nullptr`.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return nullptr; }

    /**
     * @brief Returns a const end iterator.
     * @return Always `nullptr`.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR const_iterator cend() CASTLE_CONST CASTLE_NOEXCEPT { return nullptr; }

    /**
     * @brief Returns a mutable reverse iterator to the end.
     * @return Reverse iterator constructed from `nullptr`.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR reverse_iterator rbegin() CASTLE_NOEXCEPT { return reverse_iterator(nullptr); }

    /**
     * @brief Returns a const reverse iterator to the end.
     * @return Const reverse iterator constructed from `nullptr`.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR const_reverse_iterator rbegin() CASTLE_CONST CASTLE_NOEXCEPT { return const_reverse_iterator(nullptr); }

    /**
     * @brief Returns a const reverse iterator to the end.
     * @return Const reverse iterator constructed from `nullptr`.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR const_reverse_iterator crbegin() CASTLE_CONST CASTLE_NOEXCEPT { return const_reverse_iterator(nullptr); }

    /**
     * @brief Returns a mutable reverse-end iterator.
     * @return Reverse iterator constructed from `nullptr`.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR reverse_iterator rend() CASTLE_NOEXCEPT { return reverse_iterator(nullptr); }

    /**
     * @brief Returns a const reverse-end iterator.
     * @return Const reverse iterator constructed from `nullptr`.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR const_reverse_iterator rend() CASTLE_CONST CASTLE_NOEXCEPT { return const_reverse_iterator(nullptr); }

    /**
     * @brief Returns a const reverse-end iterator.
     * @return Const reverse iterator constructed from `nullptr`.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR const_reverse_iterator crend() CASTLE_CONST CASTLE_NOEXCEPT { return const_reverse_iterator(nullptr); }

    /**
     * @brief Returns the underlying data pointer.
     * @return Always `nullptr`.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR pointer data() CASTLE_NOEXCEPT { return nullptr; }

    /**
     * @brief Returns the underlying data pointer.
     * @return Always `nullptr`.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR const_pointer data() CASTLE_CONST CASTLE_NOEXCEPT { return nullptr; }
};

}
}

#endif
