// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @brief Circular iterator adaptor for repeatedly traversing a fixed half-open range.
 *
 * Use this adaptor when embedded code must cycle through a stable `[first, last)` range
 * without modulo arithmetic or dynamic allocation. The iterator stores the supplied
 * bounds by value, wraps `++` from `last` back to `first`, and wraps `--` from `first`
 * to the element before `last`. Dereference safety, lifetime, and invalidation follow
 * the wrapped iterator and underlying storage.
 *
 * @code
 * uint8_t values[3U] = {1U, 2U, 3U};
 * castle::circular_iterator<uint8_t*> it(values, values + 3U, values);
 * ++it;
 * ++it;
 * ++it; // wraps back to values[0]
 * @endcode
 */
#ifndef CASTLE_ITERATOR_CIRCULAR_ITERATOR_HPP
#define CASTLE_ITERATOR_CIRCULAR_ITERATOR_HPP

#include "castle/core/compiler.hpp"
#include "castle/iterator/traits.hpp"

namespace castle
{

/**
 * @brief Iterator adaptor that wraps around a fixed half-open range.
 * @tparam Iterator Underlying iterator or pointer type.
 * @note The stored `first`, `last`, and `current` iterators must describe the same range.
 * @note Iterator invalidation is identical to the invalidation of the wrapped iterators.
 * @warning When `first == last`, the range is empty and the iterator must not be dereferenced.
 * @warning Equality compares only the current position, not the stored bounds.
 */
template <typename Iterator>
class circular_iterator
{
public:
    using iterator_type = Iterator;
    using iterator_category = typename iterator_traits<Iterator>::iterator_category;
    using value_type = typename iterator_traits<Iterator>::value_type;
    using difference_type = typename iterator_traits<Iterator>::difference_type;
    using pointer = typename iterator_traits<Iterator>::pointer;
    using reference = typename iterator_traits<Iterator>::reference;

    /**
     * @brief Constructs an iterator with value-initialized bounds and position.
     * @note The resulting iterator is only useful after assignment from a valid instance.
     */
    circular_iterator() CASTLE_NOEXCEPT
        : current_(), first_(), last_()
    {
    }

    /**
     * @brief Constructs an iterator over a specific circular range and position.
     * @param first Iterator to the first element in the circular range.
     * @param last Iterator one past the final element in the circular range.
     * @param current Current iterator position inside the same range.
     * @note `current` is stored verbatim; callers are responsible for supplying a position
     *       compatible with `[first, last)`.
     */
    circular_iterator(Iterator first, Iterator last, Iterator current) CASTLE_NOEXCEPT
        : current_(current), first_(first), last_(last)
    {
    }

    /**
     * @brief Constructs an iterator over a specific circular range, starting at the first element.
     * @param first Iterator to the first element in the circular range.
     * @param last Iterator one past the final element in the circular range.
     * @note The iterator will start at the first element of the range.
     * @note This constructor is equivalent to calling `circular_iterator(first, last, first)`.
     */
    circular_iterator(Iterator first, Iterator last) CASTLE_NOEXCEPT
        : current_(first), first_(first), last_(last)
    {
    }

    /**
     * @brief Returns the wrapped iterator at the current position.
     * @return The underlying iterator stored by this adaptor.
     * @note The returned iterator follows the same invalidation rules as the underlying range.
     */
    Iterator base() CASTLE_CONST CASTLE_NOEXCEPT { return current_; }

    /**
     * @brief Dereferences the current element.
     * @return Reference to the element at the current position.
     * @warning The iterator must refer to a dereferenceable element in a non-empty range.
     */
    reference operator*() CASTLE_CONST CASTLE_NOEXCEPT { return *current_; }

    /**
     * @brief Returns the address of the current element.
     * @return Pointer to the current element.
     * @warning The iterator must refer to a dereferenceable element in a non-empty range.
     */
    pointer operator->() CASTLE_CONST CASTLE_NOEXCEPT { return current_; }

    /**
     * @brief Advances to the next element, wrapping to `first` after `last`.
     * @return Reference to this iterator after incrementing.
     * @note Empty ranges remain unchanged.
     */
    circular_iterator& operator++() CASTLE_NOEXCEPT
    {
        if (first_ == last_)
        {
            return *this;
        }
        ++current_;
        if (current_ == last_)
        {
            current_ = first_;
        }
        return *this;
    }

    /**
     * @brief Returns the current iterator and then advances it.
     * @return Copy of the iterator before incrementing.
     * @note Empty ranges remain unchanged.
     */
    circular_iterator operator++(int) CASTLE_NOEXCEPT
    {
        circular_iterator temp(*this);
        ++(*this);
        return temp;
    }

    /**
     * @brief Moves to the previous element, wrapping from `first` to the element before `last`.
     * @return Reference to this iterator after decrementing.
     * @note Empty ranges remain unchanged.
     * @warning The underlying iterator type must support decrement for this operation.
     */
    circular_iterator& operator--() CASTLE_NOEXCEPT
    {
        if (first_ == last_)
        {
            return *this;
        }
        if (current_ == first_)
        {
            current_ = last_;
        }
        --current_;
        return *this;
    }

    /**
     * @brief Returns the current iterator and then decrements it.
     * @return Copy of the iterator before decrementing.
     * @note Empty ranges remain unchanged.
     * @warning The underlying iterator type must support decrement for this operation.
     */
    circular_iterator operator--(int) CASTLE_NOEXCEPT
    {
        circular_iterator temp(*this);
        --(*this);
        return temp;
    }

    /**
     * @brief Tests whether two circular iterators currently refer to the same position.
     * @param other Iterator to compare against.
     * @return `true` when both wrapped iterators compare equal; otherwise `false`.
     * @warning Stored bounds are not compared.
     */
    bool operator==(CASTLE_CONST circular_iterator& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return current_ == other.current_;
    }

    /**
     * @brief Tests whether two circular iterators refer to different positions.
     * @param other Iterator to compare against.
     * @return `true` when the wrapped iterators differ; otherwise `false`.
     * @warning Stored bounds are not compared.
     */
    bool operator!=(CASTLE_CONST circular_iterator& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return !(*this == other);
    }

private:
    Iterator current_;
    Iterator first_;
    Iterator last_;
};

}

#endif
