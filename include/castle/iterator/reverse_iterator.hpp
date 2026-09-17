// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @brief Reverse iterator adaptor for traversing a range from back to front.
 *
 * Use this adaptor when code already has a bidirectional or random-access iterator and
 * needs reverse traversal without depending on the standard library. The stored base
 * iterator follows the conventional reverse-iterator rule: it points one element past the
 * element produced by dereference. Validity, lifetime, and invalidation are inherited from
 * the wrapped iterator and underlying storage.
 *
 * @code
 * uint8_t values[3U] = {1U, 2U, 3U};
 * castle::reverse_iterator<uint8_t*> it(values + 3U);
 * uint8_t newest = *it; // reads 3
 * (void)newest;
 * @endcode
 */
#ifndef CASTLE_ITERATOR_REVERSE_ITERATOR_HPP
#define CASTLE_ITERATOR_REVERSE_ITERATOR_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/iterator/traits.hpp"

namespace castle
{

/**
 * @brief Iterator adaptor that inverts traversal direction.
 * @tparam Iterator Underlying iterator or pointer type.
 * @note Iterator invalidation is identical to the invalidation of the wrapped iterator.
 * @warning Dereferencing requires that the stored base iterator can be decremented safely.
 */
template <typename Iterator>
class reverse_iterator
{
public:
    using iterator_type = Iterator;
    using iterator_category = typename iterator_traits<Iterator>::iterator_category;
    using value_type = typename iterator_traits<Iterator>::value_type;
    using difference_type = typename iterator_traits<Iterator>::difference_type;
    using pointer = typename iterator_traits<Iterator>::pointer;
    using reference = typename iterator_traits<Iterator>::reference;

    /**
     * @brief Constructs a value-initialized reverse iterator.
     * @note The resulting iterator is only useful after assignment from a valid instance.
     */
    CASTLE_CONSTEXPR reverse_iterator() CASTLE_NOEXCEPT : current_() {}

    /**
     * @brief Constructs a reverse iterator from a base iterator.
     * @param current Base iterator stored by the adaptor.
     * @note Dereferencing accesses the element immediately preceding `current`.
     */
    explicit CASTLE_CONSTEXPR reverse_iterator(Iterator current) CASTLE_NOEXCEPT : current_(current) {}

    /**
     * @brief Constructs a reverse iterator from another compatible reverse iterator.
     * @tparam OtherIterator Other underlying iterator type.
     * @param other Reverse iterator whose base iterator will be copied.
     * @note This constructor participates when `OtherIterator` is convertible to `Iterator`.
     */
    template <typename OtherIterator>
    CASTLE_CONSTEXPR reverse_iterator(CASTLE_CONST reverse_iterator<OtherIterator>& other) CASTLE_NOEXCEPT
        : current_(other.base()) {}

    /**
     * @brief Returns the stored base iterator.
     * @return Underlying iterator one position past the element produced by dereference.
     * @note The returned iterator follows the same invalidation rules as the underlying range.
     */
    CASTLE_CONSTEXPR Iterator base() CASTLE_CONST CASTLE_NOEXCEPT { return current_; }

    /**
     * @brief Dereferences the element preceding the stored base iterator.
     * @return Reference to the current reverse element.
     * @warning The stored base iterator must be safely decrementable.
     */
    reference operator*() CASTLE_CONST CASTLE_NOEXCEPT
    {
        Iterator previous = current_;
        // Dereference the predecessor because base() points one past the reverse element.
        --previous;
        return *previous;
    }

    /**
     * @brief Returns the address of the element preceding the stored base iterator.
     * @return Pointer to the current reverse element.
     * @warning The stored base iterator must be safely decrementable.
     */
    pointer operator->() CASTLE_CONST CASTLE_NOEXCEPT
    {
        Iterator previous = current_;
        --previous;
        return previous;
    }

    /**
     * @brief Advances the reverse iterator by moving the base iterator backward.
     * @return Reference to this iterator after incrementing.
     * @warning The underlying iterator type must support decrement.
     */
    reverse_iterator& operator++() CASTLE_NOEXCEPT
    {
        --current_;
        return *this;
    }

    /**
     * @brief Returns the current reverse iterator and then advances it.
     * @return Copy of the iterator before incrementing.
     * @warning The underlying iterator type must support decrement.
     */
    reverse_iterator operator++(int) CASTLE_NOEXCEPT
    {
        reverse_iterator copy(*this);
        --current_;
        return copy;
    }

    /**
     * @brief Moves the reverse iterator backward by moving the base iterator forward.
     * @return Reference to this iterator after decrementing.
     * @warning The underlying iterator type must support increment.
     */
    reverse_iterator& operator--() CASTLE_NOEXCEPT
    {
        ++current_;
        return *this;
    }

    /**
     * @brief Returns the current reverse iterator and then decrements it.
     * @return Copy of the iterator before decrementing.
     * @warning The underlying iterator type must support increment.
     */
    reverse_iterator operator--(int) CASTLE_NOEXCEPT
    {
        reverse_iterator copy(*this);
        ++current_;
        return copy;
    }

    /**
     * @brief Returns a reverse iterator offset further into reverse traversal.
     * @param offset Number of reverse steps to add.
     * @return Reverse iterator advanced by `offset`.
     * @warning This operation requires a random-access underlying iterator.
     */
    reverse_iterator operator+(difference_type offset) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return reverse_iterator(current_ - offset);
    }

    /**
     * @brief Advances this reverse iterator by a signed reverse offset.
     * @param offset Number of reverse steps to add.
     * @return Reference to this iterator after adjustment.
     * @warning This operation requires a random-access underlying iterator.
     */
    reverse_iterator& operator+=(difference_type offset) CASTLE_NOEXCEPT
    {
        current_ -= offset;
        return *this;
    }

    /**
     * @brief Returns a reverse iterator offset toward the underlying front.
     * @param offset Number of reverse steps to subtract.
     * @return Reverse iterator moved by `offset` in the opposite direction.
     * @warning This operation requires a random-access underlying iterator.
     */
    reverse_iterator operator-(difference_type offset) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return reverse_iterator(current_ + offset);
    }

    /**
     * @brief Moves this reverse iterator toward the underlying front.
     * @param offset Number of reverse steps to subtract.
     * @return Reference to this iterator after adjustment.
     * @warning This operation requires a random-access underlying iterator.
     */
    reverse_iterator& operator-=(difference_type offset) CASTLE_NOEXCEPT
    {
        current_ += offset;
        return *this;
    }

    /**
     * @brief Accesses the element at a reverse offset from the current position.
     * @param offset Reverse offset from the current position.
     * @return Reference to the element `offset` steps away in reverse traversal order.
     * @warning This operation requires a random-access underlying iterator.
     */
    reference operator[](difference_type offset) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return *(*this + offset);
    }

private:
    Iterator current_;
};

/**
 * @brief Tests whether two reverse iterators share the same base iterator.
 * @tparam L Left-hand underlying iterator type.
 * @tparam R Right-hand underlying iterator type.
 * @param lhs Left-hand reverse iterator.
 * @param rhs Right-hand reverse iterator.
 * @return `true` when `lhs.base() == rhs.base()`; otherwise `false`.
 */
template <typename L, typename R>
bool operator==(CASTLE_CONST reverse_iterator<L>& lhs, CASTLE_CONST reverse_iterator<R>& rhs) CASTLE_NOEXCEPT
{
    return lhs.base() == rhs.base();
}

/**
 * @brief Tests whether two reverse iterators have different base iterators.
 * @tparam L Left-hand underlying iterator type.
 * @tparam R Right-hand underlying iterator type.
 * @param lhs Left-hand reverse iterator.
 * @param rhs Right-hand reverse iterator.
 * @return `true` when the base iterators differ; otherwise `false`.
 */
template <typename L, typename R>
bool operator!=(CASTLE_CONST reverse_iterator<L>& lhs, CASTLE_CONST reverse_iterator<R>& rhs) CASTLE_NOEXCEPT
{
    return !(lhs == rhs);
}

/**
 * @brief Compares two reverse iterators using reverse ordering.
 * @tparam L Left-hand underlying iterator type.
 * @tparam R Right-hand underlying iterator type.
 * @param lhs Left-hand reverse iterator.
 * @param rhs Right-hand reverse iterator.
 * @return `true` when `lhs` precedes `rhs` in reverse traversal order.
 * @warning This comparison requires `<` on the underlying iterators.
 */
template <typename L, typename R>
bool operator<(CASTLE_CONST reverse_iterator<L>& lhs, CASTLE_CONST reverse_iterator<R>& rhs) CASTLE_NOEXCEPT
{
    return rhs.base() < lhs.base();
}

/**
 * @brief Compares two reverse iterators using reverse ordering.
 * @tparam L Left-hand underlying iterator type.
 * @tparam R Right-hand underlying iterator type.
 * @param lhs Left-hand reverse iterator.
 * @param rhs Right-hand reverse iterator.
 * @return `true` when `lhs` follows `rhs` in reverse traversal order.
 * @warning This comparison requires `<` on the underlying iterators.
 */
template <typename L, typename R>
bool operator>(CASTLE_CONST reverse_iterator<L>& lhs, CASTLE_CONST reverse_iterator<R>& rhs) CASTLE_NOEXCEPT
{
    return rhs < lhs;
}

/**
 * @brief Tests whether a reverse iterator does not follow another in reverse order.
 * @tparam L Left-hand underlying iterator type.
 * @tparam R Right-hand underlying iterator type.
 * @param lhs Left-hand reverse iterator.
 * @param rhs Right-hand reverse iterator.
 * @return `true` when `lhs` is not greater than `rhs` in reverse traversal order.
 * @warning This comparison requires `<` on the underlying iterators.
 */
template <typename L, typename R>
bool operator<=(CASTLE_CONST reverse_iterator<L>& lhs, CASTLE_CONST reverse_iterator<R>& rhs) CASTLE_NOEXCEPT
{
    return !(rhs < lhs);
}

/**
 * @brief Tests whether a reverse iterator does not precede another in reverse order.
 * @tparam L Left-hand underlying iterator type.
 * @tparam R Right-hand underlying iterator type.
 * @param lhs Left-hand reverse iterator.
 * @param rhs Right-hand reverse iterator.
 * @return `true` when `lhs` is not less than `rhs` in reverse traversal order.
 * @warning This comparison requires `<` on the underlying iterators.
 */
template <typename L, typename R>
bool operator>=(CASTLE_CONST reverse_iterator<L>& lhs, CASTLE_CONST reverse_iterator<R>& rhs) CASTLE_NOEXCEPT
{
    return !(lhs < rhs);
}

/**
 * @brief Returns a reverse iterator advanced by a reverse offset.
 * @tparam Iterator Underlying iterator type.
 * @param offset Number of reverse steps to add.
 * @param iterator Reverse iterator to offset.
 * @return Reverse iterator advanced by `offset`.
 * @warning This operation requires a random-access underlying iterator.
 */
template <typename Iterator>
reverse_iterator<Iterator> operator+(
    typename reverse_iterator<Iterator>::difference_type offset,
    CASTLE_CONST reverse_iterator<Iterator>& iterator) CASTLE_NOEXCEPT
{
    return iterator + offset;
}

/**
 * @brief Computes the reverse-order distance between two reverse iterators.
 * @tparam L Left-hand underlying iterator type.
 * @tparam R Right-hand underlying iterator type.
 * @param lhs Left-hand reverse iterator.
 * @param rhs Right-hand reverse iterator.
 * @return Signed reverse distance from `rhs` to `lhs`.
 * @warning This operation requires subtraction on the underlying iterators.
 */
template <typename L, typename R>
difference_type operator-(
    CASTLE_CONST reverse_iterator<L>& lhs,
    CASTLE_CONST reverse_iterator<R>& rhs) CASTLE_NOEXCEPT
{
    return rhs.base() - lhs.base();
}

}

#endif
