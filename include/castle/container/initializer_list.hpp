// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file initializer_list.hpp
 * @brief Castle spelling of the compiler-recognized brace-init-list view type.
 *
 * Use this header when a Castle API should accept `{...}` syntax while
 * remaining allocation-free and deterministic. The type is an alias for
 * `std::initializer_list<T>` when the standard header is available and falls
 * back to a compatible freestanding definition otherwise. In both cases it is
 * a lightweight pointer/length view over a compiler-managed temporary array.
 *
 * @code
 * castle::container::initializer_list<int> values = {1, 2, 3};
 * const int first = castle::container::front(values);
 * @endcode
 */
#ifndef CASTLE_CONTAINER_INITIALIZER_LIST_HPP
#define CASTLE_CONTAINER_INITIALIZER_LIST_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/types.hpp"
#include "castle/core/traits.hpp"
#include "castle/utility/forward.hpp"

#include "castle/iterator/traits.hpp"
#include "castle/iterator/reverse_iterator.hpp"

#if CASTLE_USING_STD_INITIALIZER_LIST
#include <initializer_list>
#else

namespace std
{

/**
 * @brief Freestanding fallback for the compiler-recognized initializer-list type.
 * @tparam T Element type.
 * @note This class must remain named `std::initializer_list<T>` so the
 * compiler can bind brace-init-lists to it.
 * @warning The backing array is compiler-managed temporary storage whose
 * lifetime usually ends with the full expression that produced it.
 */
template <typename T>
class initializer_list
{
public:
    using value_type = T;
    using reference = CASTLE_CONST T&;
    using const_reference = CASTLE_CONST T&;
    using size_type = castle::size_type;
    using iterator = CASTLE_CONST T*;
    using const_iterator = CASTLE_CONST T*;

    /**
     * @brief Constructs an empty initializer list.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR initializer_list() CASTLE_NOEXCEPT : first_(nullptr), size_(0U) {}

    /**
     * @brief Returns the number of elements.
     * @return Element count.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return size_; }

    /**
     * @brief Returns an iterator to the first element.
     * @return Pointer to the first element, or `nullptr` when empty.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR CASTLE_CONST T* begin() CASTLE_CONST CASTLE_NOEXCEPT { return first_; }

    /**
     * @brief Returns an iterator one past the last element.
     * @return Pointer one past the last element.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR CASTLE_CONST T* end() CASTLE_CONST CASTLE_NOEXCEPT { return first_ + size_; }

private:
    CASTLE_CONSTEXPR initializer_list(CASTLE_CONST T* first, size_type size) CASTLE_NOEXCEPT
        : first_(first), size_(size)
    {
    }

    CASTLE_CONST T* first_;
    size_type size_;
};

}

#endif

namespace castle
{
namespace container
{

/**
 * @brief Alias used by Castle APIs to accept brace-init-lists.
 * @tparam T Element type.
 * @note This resolves to `std::initializer_list<T>` or a compatible
 * freestanding fallback with the same semantics.
 * @warning The backing array is non-owning temporary storage whose lifetime
 * usually ends with the full expression that created it.
 */
template <typename T>
using initializer_list = CASTLE_STD::initializer_list<T>;

/**
 * @brief Reverse iterator type for `initializer_list<T>`.
 * @tparam T Element type.
 * @note This is a `castle::reverse_iterator<const T*>`.
 */
template <typename T>
using const_reverse_initializer_iterator = castle::reverse_iterator<CASTLE_CONST T*>;

/**
 * @brief Returns an iterator to the first element.
 * @tparam T Element type.
 * @param list Initializer list to inspect.
 * @return Pointer to the first element.
 * @note Complexity: O(1).
 */
template <typename T>
CASTLE_CONSTEXPR CASTLE_CONST T* begin(initializer_list<T> list) CASTLE_NOEXCEPT
{
    return list.begin();
}

/**
 * @brief Returns an iterator one past the last element.
 * @tparam T Element type.
 * @param list Initializer list to inspect.
 * @return Pointer one past the last element.
 * @note Complexity: O(1).
 */
template <typename T>
CASTLE_CONSTEXPR CASTLE_CONST T* end(initializer_list<T> list) CASTLE_NOEXCEPT
{
    return list.end();
}

/**
 * @brief Returns the number of elements.
 * @tparam T Element type.
 * @param list Initializer list to inspect.
 * @return Element count.
 * @note Complexity: O(1).
 */
template <typename T>
CASTLE_CONSTEXPR castle::size_type size(initializer_list<T> list) CASTLE_NOEXCEPT
{
    return list.size();
}

/**
 * @brief Reports whether the initializer list is empty.
 * @tparam T Element type.
 * @param list Initializer list to inspect.
 * @return `true` when `list.size() == 0`, otherwise `false`.
 * @note Complexity: O(1).
 */
template <typename T>
CASTLE_CONSTEXPR bool empty(initializer_list<T> list) CASTLE_NOEXCEPT
{
    return list.size() == 0U;
}

/**
 * @brief Returns the first element.
 * @tparam T Element type.
 * @param list Initializer list to inspect.
 * @return Const reference to the first element.
 * @note Complexity: O(1).
 * @warning `list` must not be empty.
 */
template <typename T>
CASTLE_CONSTEXPR CASTLE_CONST T& front(initializer_list<T> list) CASTLE_NOEXCEPT
{
    return *list.begin();
}

/**
 * @brief Returns the last element.
 * @tparam T Element type.
 * @param list Initializer list to inspect.
 * @return Const reference to the last element.
 * @note Complexity: O(1).
 * @warning `list` must not be empty.
 */
template <typename T>
CASTLE_CONSTEXPR CASTLE_CONST T& back(initializer_list<T> list) CASTLE_NOEXCEPT
{
    return *(list.end() - 1);
}

/**
 * @brief Returns a reverse iterator to the last element.
 * @tparam T Element type.
 * @param list Initializer list to inspect.
 * @return Reverse iterator to the last element.
 * @note Complexity: O(1).
 */
template <typename T>
CASTLE_CONSTEXPR const_reverse_initializer_iterator<T> rbegin(initializer_list<T> list) CASTLE_NOEXCEPT
{
    return const_reverse_initializer_iterator<T>(list.end());
}

/**
 * @brief Returns a reverse-end iterator.
 * @tparam T Element type.
 * @param list Initializer list to inspect.
 * @return Reverse iterator one element before the first element.
 * @note Complexity: O(1).
 */
template <typename T>
CASTLE_CONSTEXPR const_reverse_initializer_iterator<T> rend(initializer_list<T> list) CASTLE_NOEXCEPT
{
    return const_reverse_initializer_iterator<T>(list.begin());
}

}
}

#endif
