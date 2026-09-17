// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @brief Bounded iterator adaptor that clamps movement to a fixed half-open range.
 *
 * Use this adaptor when embedded code must expose iterator-style traversal over a stable
 * `[first, last)` range while preventing accidental increments past `last` or decrements
 * before `first`. The adaptor stores the bounds and current position by value. It does not
 * own storage, allocate memory, or repair invalid underlying iterators.
 *
 * @code
 * uint8_t values[4U] = {1U, 2U, 3U, 4U};
 * castle::fixed_iterator<uint8_t*> it(values, values, values + 4U);
 * while (it.valid())
 * {
 *     ++it;
 * }
 * @endcode
 */
#ifndef CASTLE_ITERATOR_FIXED_ITERATOR_HPP
#define CASTLE_ITERATOR_FIXED_ITERATOR_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/iterator/traits.hpp"

namespace castle
{

/**
 * @brief Iterator adaptor that never moves outside a configured half-open range.
 * @tparam Iterator Underlying iterator or pointer type.
 * @note Iterator invalidation is identical to the invalidation of the wrapped iterators.
 * @note Equality compares only the current position, not the stored bounds.
 * @warning Dereferencing is only valid while `valid()` returns `true`.
 */
template <typename Iterator>
class fixed_iterator
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
    fixed_iterator() CASTLE_NOEXCEPT
        : current_(), first_(), last_()
    {
    }

    /**
     * @brief Constructs an iterator over a bounded half-open range.
     * @param first Iterator to the first valid element.
     * @param current Current iterator position, typically inside `[first, last]`.
     * @param last Iterator one past the final valid element.
     * @note `current == last` represents the end sentinel and makes `valid()` return `false`.
     */
    fixed_iterator(Iterator first, Iterator current, Iterator last) CASTLE_NOEXCEPT
        : current_(current), first_(first), last_(last)
    {
    }

    /**
     * @brief Constructs an iterator over a bounded half-open range, starting at the first element.
     * @param first Iterator to the first valid element.
     * @param last Iterator one past the final valid element.
     * @note The iterator will start at the first element of the range.
     * @note This constructor is equivalent to calling `fixed_iterator(first, first, last)`.
     */
    fixed_iterator(Iterator first, Iterator last) CASTLE_NOEXCEPT
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
     * @brief Reports whether the current position is dereferenceable.
     * @return `true` when `current_ != last_`; otherwise `false`.
     * @note A `false` result indicates the iterator is at the stored end sentinel.
     */
    bool valid() CASTLE_CONST CASTLE_NOEXCEPT { return current_ != last_; }

    /**
     * @brief Dereferences the current element.
     * @return Reference to the element at the current position.
     * @warning The iterator must be valid before dereferencing.
     */
    reference operator*() CASTLE_CONST CASTLE_NOEXCEPT { return *current_; }

    /**
     * @brief Returns the address of the current element.
     * @return Pointer to the current element.
     * @warning The iterator must be valid before dereferencing.
     */
    pointer operator->() CASTLE_CONST CASTLE_NOEXCEPT { return current_; }

    /**
     * @brief Advances toward `last` without moving past it.
     * @return Reference to this iterator after incrementing.
     * @note If the iterator is already at `last`, the position is unchanged.
     */
    fixed_iterator& operator++() CASTLE_NOEXCEPT
    {
        if (current_ != last_)
        {
            ++current_;
        }
        return *this;
    }

    /**
     * @brief Returns the current iterator and then advances it toward `last`.
     * @return Copy of the iterator before incrementing.
     * @note If the iterator is already at `last`, the position is unchanged.
     */
    fixed_iterator operator++(int) CASTLE_NOEXCEPT
    {
        fixed_iterator temp(*this);
        ++(*this);
        return temp;
    }

    /**
     * @brief Moves toward `first` without moving before it.
     * @return Reference to this iterator after decrementing.
     * @note A stored end sentinel can be decremented back into the range when `first != last`.
     * @warning The underlying iterator type must support decrement for this operation.
     */
    fixed_iterator& operator--() CASTLE_NOEXCEPT
    {
        if (current_ != first_)
        {
            --current_;
        }
        return *this;
    }

    /**
     * @brief Returns the current iterator and then decrements it toward `first`.
     * @return Copy of the iterator before decrementing.
     * @note A stored end sentinel can be decremented back into the range when `first != last`.
     * @warning The underlying iterator type must support decrement for this operation.
     */
    fixed_iterator operator--(int) CASTLE_NOEXCEPT
    {
        fixed_iterator temp(*this);
        --(*this);
        return temp;
    }

    /**
     * @brief Tests whether two fixed iterators currently refer to the same position.
     * @param other Iterator to compare against.
     * @return `true` when both wrapped iterators compare equal; otherwise `false`.
     * @warning Stored bounds are not compared.
     */
    bool operator==(CASTLE_CONST fixed_iterator& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return current_ == other.current_;
    }

    /**
     * @brief Tests whether two fixed iterators refer to different positions.
     * @param other Iterator to compare against.
     * @return `true` when the wrapped iterators differ; otherwise `false`.
     * @warning Stored bounds are not compared.
     */
    bool operator!=(CASTLE_CONST fixed_iterator& other) CASTLE_CONST CASTLE_NOEXCEPT
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
