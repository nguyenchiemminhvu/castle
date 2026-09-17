// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file vector.hpp
 * @brief Fixed-capacity contiguous sequence with explicit lifetime management for live elements.
 *
 * Use this container when a deterministic, heap-free growable array is needed and the maximum
 * element count is known at compile time. Storage for `N` elements is embedded directly in the
 * object, only the first `size()` slots contain live `T` objects, and appends/removals at the
 * back are O(1). Copy and clear operations are O(size()).
 *
 * @note The container never reallocates, throws, or depends on the STL.
 * @warning `push_back()` and `emplace_back()` return `status::full` when capacity is exhausted.
 */
#ifndef CASTLE_CONTAINER_VECTOR_HPP
#define CASTLE_CONTAINER_VECTOR_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/container/initializer_list.hpp"
#include "castle/error/status.hpp"
#include "castle/iterator/reverse_iterator.hpp"
#include "castle/memory/object.hpp"
#include "castle/memory/construct.hpp"
#include "castle/memory/destroy.hpp"
#include "castle/memory/object.hpp"
#include "castle/memory/static_storage.hpp"
#include "castle/utility/forward.hpp"
#include "castle/utility/move.hpp"
#include <stddef.h>

namespace castle
{
namespace container
{

/**
 * @brief Fixed-capacity vector with in-object storage.
 * @tparam T Element type.
 * @tparam N Maximum number of elements stored at once.
 * @note `size()` live objects occupy the first slots of the internal storage. Element addresses
 * remain stable until the element is removed or the container is cleared.
 * @warning Capacity is fixed at compile time; no dynamic growth is performed.
 */
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

    /** @brief Compile-time element capacity. */
    static CASTLE_CONSTEXPR size_type static_capacity = N;

    /**
     * @brief Constructs an empty vector.
     * @note Complexity is O(1).
     */
    vector() CASTLE_NOEXCEPT : size_(0U) {}

    /**
     * @brief Destroys all live elements.
     * @note Complexity is O(size()).
     */
    ~vector() CASTLE_NOEXCEPT
    {
        clear();
    }

    /**
     * @brief Copy-constructs the vector from another vector.
     * @param other Source vector.
     * @note Complexity is O(other.size()).
     * @warning Requires `T` to be copy constructible.
     */
    vector(CASTLE_CONST vector& other) : size_(0U)
    {
        static_assert(meta::is_copy_constructible<T>::value,
                      "vector copy construction requires a copy-constructible T");
        for (size_type i = 0U; i < other.size_; ++i)
        {
            memory::construct_at<T>(storage_.address(i), other[i]);
            ++size_;
        }
    }

    /**
     * @brief Replaces the vector contents with a copy of another vector.
     * @param other Source vector.
     * @return Reference to `*this`.
     * @note Complexity is O(size() + other.size()).
     * @warning Existing elements are destroyed before copying begins.
     */
    vector& operator=(CASTLE_CONST vector& other)
    {
        if (this != &other) // LCOV_EXCL_BR_LINE
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

    /**
     * @brief Move-constructs the vector from another vector.
     * @param other Source vector.
     * @note Complexity is O(other.size()).
     * @warning Requires `T` to be move constructible.
     */
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

    /**
     * @brief Replaces the vector contents by moving from another vector.
     * @param other Source vector.
     * @return Reference to `*this`.
     * @note Complexity is O(size() + other.size()).
     * @warning Existing elements are destroyed before moved-in elements are constructed.
     */
    vector& operator=(vector&& other) CASTLE_NOEXCEPT(meta::is_nothrow_move_constructible<T>::value)
    {
        if (this != &other) // LCOV_EXCL_BR_LINE
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

    /**
     * @brief Constructs the vector from a brace-initializer list.
     * @param list Source elements.
     * @note Complexity is O(list.size()).
     * @warning Oversized initializer lists trigger `CASTLE_ASSERT`.
     */
    vector(initializer_list<T> list) : size_(0U)
    {
        static_assert(meta::is_copy_constructible<T>::value,
                      "vector initializer_list construction requires a copy-constructible T");
        for (CASTLE_CONST T& value : list)
        {
            if (size_ >= N)
            {
                break;
            }
            memory::construct_at<T>(storage_.address(size_), value);
            ++size_;
        }
    }

    /**
     * @brief Replaces the vector contents from a brace-initializer list.
     * @param list Source elements.
     * @return Reference to `*this`.
     * @note Complexity is O(size() + list.size()).
     * @warning Oversized initializer lists trigger `CASTLE_ASSERT`.
     */
    vector& operator=(initializer_list<T> list)
    {
        static_assert(meta::is_copy_constructible<T>::value,
                      "vector initializer_list assignment requires a copy-constructible T");
        clear();
        for (CASTLE_CONST T& value : list)
        {
            if (size_ >= N)
            {
                break;
            }
            memory::construct_at<T>(storage_.address(size_), value);
            ++size_;
        }
        return *this;
    }

    /**
     * @brief Returns the number of live elements.
     * @return Current element count in O(1).
     */
    CASTLE_CONSTEXPR size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return size_; }

    /**
     * @brief Returns the fixed compile-time capacity.
     * @return `N`.
     */
    CASTLE_CONSTEXPR size_type capacity() CASTLE_CONST CASTLE_NOEXCEPT { return N; }

    /**
     * @brief Tests whether the vector has no elements.
     * @return `true` when `size() == 0`.
     */
    CASTLE_CONSTEXPR bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == 0U; }

    /**
     * @brief Tests whether the vector is at capacity.
     * @return `true` when `size() == capacity()`.
     */
    CASTLE_CONSTEXPR bool full() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == N; }

    /**
     * @brief Returns an iterator to the first element.
     * @return Pointer to the first storage slot.
     * @note For an empty vector this still returns a valid storage pointer so `begin() == end()`
     * remains well-defined without null-pointer arithmetic.
     */
    iterator begin() CASTLE_NOEXCEPT
    {
        return pointer_at(0U);
    }

    /**
     * @brief Returns a const iterator to the first element.
     * @return Pointer to the first storage slot.
     */
    const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return pointer_at(0U);
    }

    /**
     * @brief Returns a const iterator to the first element.
     * @return Const iterator equal to `begin()`.
     */
    const_iterator cbegin() CASTLE_CONST CASTLE_NOEXCEPT { return begin(); }

    /**
     * @brief Returns an iterator one past the last element.
     * @return End iterator.
     */
    iterator end() CASTLE_NOEXCEPT
    {
        return begin() + size_;
    }

    /**
     * @brief Returns a const iterator one past the last element.
     * @return Const end iterator.
     */
    const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return begin() + size_;
    }

    /**
     * @brief Returns a const iterator one past the last element.
     * @return Const end iterator equal to `end()`.
     */
    const_iterator cend() CASTLE_CONST CASTLE_NOEXCEPT { return end(); }

    /**
     * @brief Returns a reverse iterator to the last element.
     * @return Reverse iterator constructed from `end()`.
     */
    reverse_iterator rbegin() CASTLE_NOEXCEPT { return reverse_iterator(end()); }

    /**
     * @brief Returns a const reverse iterator to the last element.
     * @return Const reverse iterator constructed from `end()`.
     */
    const_reverse_iterator rbegin() CASTLE_CONST CASTLE_NOEXCEPT { return const_reverse_iterator(end()); }

    /**
     * @brief Returns a const reverse iterator to the last element.
     * @return Const reverse iterator equal to `rbegin()`.
     */
    const_reverse_iterator crbegin() CASTLE_CONST CASTLE_NOEXCEPT { return const_reverse_iterator(end()); }

    /**
     * @brief Returns a reverse iterator one before the first element.
     * @return Reverse end iterator.
     */
    reverse_iterator rend() CASTLE_NOEXCEPT { return reverse_iterator(begin()); }

    /**
     * @brief Returns a const reverse iterator one before the first element.
     * @return Const reverse end iterator.
     */
    const_reverse_iterator rend() CASTLE_CONST CASTLE_NOEXCEPT { return const_reverse_iterator(begin()); }

    /**
     * @brief Returns a const reverse iterator one before the first element.
     * @return Const reverse end iterator equal to `rend()`.
     */
    const_reverse_iterator crend() CASTLE_CONST CASTLE_NOEXCEPT { return const_reverse_iterator(begin()); }

    /**
     * @brief Returns the element at `index` without bounds checking.
     * @param index Zero-based element index.
     * @return Reference to the requested element in O(1).
     * @warning `index` must be less than `size()`.
     */
    reference operator[](size_type index) CASTLE_NOEXCEPT
    {
        return *pointer_at(index);
    }

    /**
     * @brief Returns the element at `index` without bounds checking.
     * @param index Zero-based element index.
     * @return Const reference to the requested element in O(1).
     * @warning `index` must be less than `size()`.
     */
    const_reference operator[](size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return *pointer_at(index);
    }

    /**
     * @brief Returns the first element.
     * @return Reference to `operator[](0)`.
     * @warning The vector must not be empty.
     */
    reference front() CASTLE_NOEXCEPT { return *begin(); }

    /**
     * @brief Returns the first element.
     * @return Const reference to `operator[](0)`.
     * @warning The vector must not be empty.
     */
    const_reference front() CASTLE_CONST CASTLE_NOEXCEPT { return *begin(); }

    /**
     * @brief Returns the last element.
     * @return Reference to the last live element.
     * @warning The vector must not be empty.
     */
    reference back() CASTLE_NOEXCEPT { return *(end() - 1); }

    /**
     * @brief Returns the last element.
     * @return Const reference to the last live element.
     * @warning The vector must not be empty.
     */
    const_reference back() CASTLE_CONST CASTLE_NOEXCEPT { return *(end() - 1); }

    /**
     * @brief Returns a pointer to the contiguous storage.
     * @return Pointer to the first storage slot.
     * @note The first `size()` elements are live objects.
     */
    pointer data() CASTLE_NOEXCEPT { return pointer_at(0U); }

    /**
     * @brief Returns a const pointer to the contiguous storage.
     * @return Const pointer to the first storage slot.
     */
    const_pointer data() CASTLE_CONST CASTLE_NOEXCEPT { return pointer_at(0U); }

    /**
     * @brief Constructs an element at the end of the vector.
     * @tparam Args Constructor argument types.
     * @param args Arguments forwarded to `T`.
     * @return `status::ok` on success or `status::full` when no slot is available.
     * @note Complexity is O(1).
     */
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

    /**
     * @brief Appends a copy of `value`.
     * @param value Element to append.
     * @return `status::ok` on success or `status::full` when no slot is available.
     * @note Complexity is O(1).
     */
    status push_back(CASTLE_CONST T& value)
        CASTLE_NOEXCEPT(meta::is_nothrow_constructible<T, CASTLE_CONST T&>::value)
    {
        return emplace_back(value);
    }

    /**
     * @brief Appends `value` by move construction.
     * @param value Element to append.
     * @return `status::ok` on success or `status::full` when no slot is available.
     * @note Complexity is O(1).
     */
    status push_back(T&& value)
        CASTLE_NOEXCEPT(meta::is_nothrow_constructible<T, T&&>::value)
    {
        return emplace_back(CASTLE_MOVE(value));
    }

    /**
     * @brief Removes the last element.
     * @return `status::ok` on success or `status::empty` when the vector has no elements.
     * @note Complexity is O(1).
     */
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

    /**
     * @brief Destroys all live elements and resets the size to zero.
     * @note Complexity is O(size()).
     */
    void clear() CASTLE_NOEXCEPT
    {
        memory::destroy_n(pointer_at(0U), size_);
        size_ = 0U;
    }

private:
    /**
     * @brief Returns a pointer to the element at the specified index.
     * @param index Index of the element.
     * @return Pointer to the element at the specified index.
     */
    pointer pointer_at(size_type index) CASTLE_NOEXCEPT
    {
        return memory::object_from_address<T>(storage_.address(index));
    }

    /**
     * @brief Returns a const pointer to the element at the specified index.
     * @param index Index of the element.
     * @return Const pointer to the element at the specified index.
     */
    const_pointer pointer_at(size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return memory::object_from_address<CASTLE_CONST T>(storage_.address(index));
    }

    size_type size_;
    memory::static_storage<T, N> storage_;
};

} // namespace container
} // namespace castle
#endif // CASTLE_CONTAINER_VECTOR_HPP
