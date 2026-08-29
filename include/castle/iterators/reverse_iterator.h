#pragma once

#include "castle/iterators/iterator_traits.h"

#include <type_traits>

// ─────────────────────────────────────────────────────────────────────────────
// reverse_iterator — traverses any bidirectional Castle iterator backwards.
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Semantics match std::reverse_iterator:
 *
 *     rbegin() == reverse_iterator(end())
 *     rend()   == reverse_iterator(begin())
 *
 * Dereferencing yields the element preceding the stored base iterator, so the
 * wrapped iterator must be at least bidirectional. Random-access affordances
 * (+, -, +=, [], ordering) are enabled only when the underlying iterator is
 * itself random-access.
 */

namespace castle
{
namespace iterators
{

template <typename Iterator>
class reverse_iterator
{
private:
    using traits = iterator_traits<Iterator>;

    template <typename I>
    using enable_if_random_access =
        typename std::enable_if<
            std::is_base_of<
                random_access_iterator_tag,
                typename iterator_traits<I>::iterator_category>::value,
            int>::type;

public:
    using iterator_type     = Iterator;
    using iterator_category = typename traits::iterator_category;
    using value_type        = typename traits::value_type;
    using difference_type   = typename traits::difference_type;
    using pointer           = typename traits::pointer;
    using reference         = typename traits::reference;

    constexpr reverse_iterator() noexcept : current_() {}

    explicit constexpr reverse_iterator(Iterator it) noexcept : current_(it) {}

    // Allow reverse_iterator<iterator> -> reverse_iterator<const_iterator>.
    template <typename OtherIterator>
    constexpr reverse_iterator(const reverse_iterator<OtherIterator>& other) noexcept
        : current_(other.base())
    {
    }

    constexpr Iterator base() const noexcept { return current_; }

    reference operator*() const noexcept
    {
        Iterator tmp = current_;
        --tmp;
        return *tmp;
    }

    pointer operator->() const noexcept { return &operator*(); }

    // ── traversal ───────────────────────────────────────────────────────────

    reverse_iterator& operator++() noexcept { --current_; return *this; }

    reverse_iterator operator++(int) noexcept
    {
        reverse_iterator copy(*this);
        --current_;
        return copy;
    }

    reverse_iterator& operator--() noexcept { ++current_; return *this; }

    reverse_iterator operator--(int) noexcept
    {
        reverse_iterator copy(*this);
        ++current_;
        return copy;
    }

    // ── random-access movement (enabled for random-access base only) ──────────

    template <typename I = Iterator, enable_if_random_access<I> = 0>
    reverse_iterator& operator+=(difference_type n) noexcept { current_ -= n; return *this; }

    template <typename I = Iterator, enable_if_random_access<I> = 0>
    reverse_iterator& operator-=(difference_type n) noexcept { current_ += n; return *this; }

    template <typename I = Iterator, enable_if_random_access<I> = 0>
    reverse_iterator operator+(difference_type n) const noexcept
    {
        reverse_iterator result(*this);
        result += n;
        return result;
    }

    template <typename I = Iterator, enable_if_random_access<I> = 0>
    reverse_iterator operator-(difference_type n) const noexcept
    {
        reverse_iterator result(*this);
        result -= n;
        return result;
    }

    template <typename I = Iterator, enable_if_random_access<I> = 0>
    reference operator[](difference_type n) const noexcept { return *(*this + n); }

private:
    Iterator current_;
};

// ── n + reverse_iterator ─────────────────────────────────────────────────────

template <
    typename Iterator,
    typename std::enable_if<
        std::is_base_of<
            random_access_iterator_tag,
            typename iterator_traits<Iterator>::iterator_category>::value,
        int>::type = 0>
reverse_iterator<Iterator> operator+(
    typename reverse_iterator<Iterator>::difference_type n,
    const reverse_iterator<Iterator>& it) noexcept
{
    return it + n;
}

// ── comparisons (cross const/non-const via base()) ───────────────────────────

template <typename Iterator, typename OtherIterator>
bool operator==(const reverse_iterator<Iterator>& lhs,
                const reverse_iterator<OtherIterator>& rhs) noexcept
{
    return lhs.base() == rhs.base();
}

template <typename Iterator, typename OtherIterator>
bool operator!=(const reverse_iterator<Iterator>& lhs,
                const reverse_iterator<OtherIterator>& rhs) noexcept
{
    return !(lhs == rhs);
}

// Reversing swaps the ordering relation of the underlying bases.
template <typename Iterator, typename OtherIterator>
bool operator<(const reverse_iterator<Iterator>& lhs,
               const reverse_iterator<OtherIterator>& rhs) noexcept
{
    return rhs.base() < lhs.base();
}

template <typename Iterator, typename OtherIterator>
bool operator>(const reverse_iterator<Iterator>& lhs,
               const reverse_iterator<OtherIterator>& rhs) noexcept
{
    return rhs < lhs;
}

template <typename Iterator, typename OtherIterator>
bool operator<=(const reverse_iterator<Iterator>& lhs,
                const reverse_iterator<OtherIterator>& rhs) noexcept
{
    return !(rhs < lhs);
}

template <typename Iterator, typename OtherIterator>
bool operator>=(const reverse_iterator<Iterator>& lhs,
                const reverse_iterator<OtherIterator>& rhs) noexcept
{
    return !(lhs < rhs);
}

// Distance between two reverse iterators (base order is reversed).
template <typename Iterator, typename OtherIterator>
auto operator-(const reverse_iterator<Iterator>& lhs,
               const reverse_iterator<OtherIterator>& rhs) noexcept
    -> decltype(rhs.base() - lhs.base())
{
    return rhs.base() - lhs.base();
}

} // namespace iterators
} // namespace castle
