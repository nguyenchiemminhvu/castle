// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file array_view.hpp
 * @brief Non-owning view over contiguous storage.
 *
 * Use `array_view<T>` to pass a pointer/length pair through APIs without
 * copying elements or transferring ownership. The view stores only a pointer
 * and a size, performs no allocation, and offers O(1) size queries, indexed
 * access, iterator access, and subview creation.
 *
 * @code
 * int samples[4] = {1, 2, 3, 4};
 * castle::container::array_view<int> view(samples, 4U);
 * castle::container::array_view<int> tail = view.subview(2U, 2U);
 * @endcode
 */
#ifndef CASTLE_CONTAINER_ARRAY_VIEW_HPP
#define CASTLE_CONTAINER_ARRAY_VIEW_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/initializer_list.hpp"

namespace castle
{
namespace container
{

/**
 * @brief Non-owning view over contiguous elements.
 * @tparam T Element type. Use `const T` for a read-only view.
 * @note The view stores only a pointer and a length. All operations are O(1).
 * @warning The referenced storage must outlive the view.
 */
template <typename T>
class array_view
{
public:
    using value_type = T;
    using size_type = castle::size_type;
    using difference_type = castle::difference_type;
    using pointer = T*;
    using const_pointer = CASTLE_CONST T*;
    using reference = T&;
    using const_reference = CASTLE_CONST T&;
    using iterator = T*;
    using const_iterator = CASTLE_CONST T*;

    /**
     * @brief Constructs an empty view.
     * @note Complexity: O(1).
     */
    array_view() CASTLE_NOEXCEPT : data_(nullptr), size_(0U) {}

    /**
     * @brief Constructs a view from a raw pointer and element count.
     * @param data Pointer to the first element, or `nullptr` for an empty view.
     * @param size Number of elements in the referenced range.
     * @note Complexity: O(1).
     * @warning No ownership is taken; `data` must remain valid for the view's lifetime.
     */
    array_view(pointer data, size_type size) CASTLE_NOEXCEPT
        : data_(data), size_(size)
    {
    }

    /**
     * @brief Constructs a view from a compatible mutable container.
     * @tparam Container Container type exposing `data()` and `size()`.
     * @param container Source container whose storage is referenced.
     * @note Complexity: O(1).
     * @warning The constructor is only well-formed when `container.data()` is
     * convertible to `pointer`.
     */
    template <typename Container>
    explicit array_view(Container& container) CASTLE_NOEXCEPT
        : data_(container.data()), size_(container.size())
    {
    }

    /**
     * @brief Constructs a view from a compatible const container.
     * @tparam Container Container type exposing `data()` and `size()`.
     * @param container Source container whose storage is referenced.
     * @note Complexity: O(1).
     * @warning The constructor is only well-formed when `container.data()` is
     * convertible to `pointer`.
     */
    template <typename Container>
    explicit array_view(CASTLE_CONST Container& container) CASTLE_NOEXCEPT
        : data_(container.data()), size_(container.size())
    {
    }

    /**
     * @brief Returns the number of elements in the view.
     * @return Referenced element count.
     * @note Complexity: O(1).
     */
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return size_; }

    /**
     * @brief Reports whether the view is empty.
     * @return `true` when `size() == 0`, otherwise `false`.
     * @note Complexity: O(1).
     */
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == 0U; }

    /**
     * @brief Returns the element at `index`.
     * @param index Zero-based element index.
     * @return Reference to the indexed element.
     * @note Complexity: O(1).
     * @warning `index` must be less than `size()`.
     */
    reference operator[](size_type index) CASTLE_CONST CASTLE_NOEXCEPT { return data_[index]; }

    /**
     * @brief Returns the first element.
     * @return Reference to the first element.
     * @note Complexity: O(1).
     * @warning The view must not be empty.
     */
    reference front() CASTLE_CONST CASTLE_NOEXCEPT { return data_[0U]; }

    /**
     * @brief Returns the last element.
     * @return Reference to the last element.
     * @note Complexity: O(1).
     * @warning The view must not be empty.
     */
    reference back() CASTLE_CONST CASTLE_NOEXCEPT { return data_[size_ - 1U]; }

    /**
     * @brief Returns the underlying data pointer.
     * @return Pointer to the first element, or `nullptr` for the default-constructed empty view.
     * @note Complexity: O(1).
     */
    pointer data() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }

    /**
     * @brief Returns an iterator to the first element.
     * @return Iterator to the first element.
     * @note Complexity: O(1).
     */
    iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }

    /**
     * @brief Returns an iterator one past the last element.
     * @return End iterator for the viewed range.
     * @note Complexity: O(1).
     */
    iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == 0U ? data_ : data_ + size_; }

    /**
     * @brief Returns a bounded child view.
     * @param offset Zero-based starting offset within the current view.
     * @param count Requested number of elements in the child view.
     * @return A view covering `[offset, offset + count)` clamped to the
     * available range, or an empty default-constructed view when `offset > size()`.
     * @note Complexity: O(1).
     * @warning The returned view aliases the same storage as the parent view.
     */
    array_view subview(size_type offset, size_type count) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (offset > size_)
        {
            return array_view();
        }
        CASTLE_CONST size_type available = size_ - offset;
        if (count > available)
        {
            count = available;
        }
        return array_view(
            count == 0U
            ? data_
            : data_ + offset, count
        );
    }

private:
    pointer data_;
    size_type size_;
};

/**
 * @brief Creates a mutable array view from a raw pointer and element count.
 * @tparam T Element type.
 * @param data Pointer to the first element.
 * @param size Number of elements in the view.
 * @return `array_view<T>(data, size)`.
 * @note Complexity: O(1).
 * @warning The caller retains ownership of the referenced storage.
 */
template <typename T>
array_view<T> make_array_view(T* data, size_type size) CASTLE_NOEXCEPT
{
    return array_view<T>(data, size);
}

/**
 * @brief Creates a read-only array view from a raw pointer and element count.
 * @tparam T Element type.
 * @param data Pointer to the first element.
 * @param size Number of elements in the view.
 * @return `array_view<const T>(data, size)`.
 * @note Complexity: O(1).
 * @warning The caller retains ownership of the referenced storage.
 */
template <typename T>
array_view<CASTLE_CONST T> make_array_view(CASTLE_CONST T* data, size_type size) CASTLE_NOEXCEPT
{
    return array_view<CASTLE_CONST T>(data, size);
}

/**
 * @brief Creates a read-only view over an initializer list.
 * @tparam T Element type.
 * @param list Initializer list whose elements are viewed.
 * @return A read-only array view over the initializer-list backing array.
 * @note Complexity: O(1).
 * @warning The returned view is only valid for the lifetime of the
 * initializer-list temporary, typically the current full expression.
 */
template <typename T>
array_view<CASTLE_CONST T> make_array_view(initializer_list<T> list) CASTLE_NOEXCEPT
{
    return array_view<CASTLE_CONST T>(list.begin(), list.size());
}

}
}

#endif
