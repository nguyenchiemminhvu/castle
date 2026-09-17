#ifndef CASTLE_ITERATOR_REVERSE_ITERATOR_H
#define CASTLE_ITERATOR_REVERSE_ITERATOR_H

#include "castle/core/compiler.h"
#include "castle/core/types.h"
#include "castle/iterator/traits.h"

namespace castle
{

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

    CASTLE_CONSTEXPR reverse_iterator() CASTLE_NOEXCEPT : current_() {}
    explicit CASTLE_CONSTEXPR reverse_iterator(Iterator current) CASTLE_NOEXCEPT : current_(current) {}

    template <typename OtherIterator>
    CASTLE_CONSTEXPR reverse_iterator(CASTLE_CONST reverse_iterator<OtherIterator>& other) CASTLE_NOEXCEPT
        : current_(other.base()) {}

    CASTLE_CONSTEXPR Iterator base() CASTLE_CONST CASTLE_NOEXCEPT { return current_; }

    reference operator*() CASTLE_CONST CASTLE_NOEXCEPT
    {
        Iterator previous = current_;
        --previous;
        return *previous;
    }

    pointer operator->() CASTLE_CONST CASTLE_NOEXCEPT
    {
        Iterator previous = current_;
        --previous;
        return previous;
    }

    reverse_iterator& operator++() CASTLE_NOEXCEPT
    {
        --current_;
        return *this;
    }

    reverse_iterator operator++(int) CASTLE_NOEXCEPT
    {
        reverse_iterator copy(*this);
        --current_;
        return copy;
    }

    reverse_iterator& operator--() CASTLE_NOEXCEPT
    {
        ++current_;
        return *this;
    }

    reverse_iterator operator--(int) CASTLE_NOEXCEPT
    {
        reverse_iterator copy(*this);
        ++current_;
        return copy;
    }

    reverse_iterator operator+(difference_type offset) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return reverse_iterator(current_ - offset);
    }

    reverse_iterator& operator+=(difference_type offset) CASTLE_NOEXCEPT
    {
        current_ -= offset;
        return *this;
    }

    reverse_iterator operator-(difference_type offset) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return reverse_iterator(current_ + offset);
    }

    reverse_iterator& operator-=(difference_type offset) CASTLE_NOEXCEPT
    {
        current_ += offset;
        return *this;
    }

    reference operator[](difference_type offset) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return *(*this + offset);
    }

private:
    Iterator current_;
};

template <typename L, typename R>
bool operator==(CASTLE_CONST reverse_iterator<L>& lhs, CASTLE_CONST reverse_iterator<R>& rhs) CASTLE_NOEXCEPT
{
    return lhs.base() == rhs.base();
}

template <typename L, typename R>
bool operator!=(CASTLE_CONST reverse_iterator<L>& lhs, CASTLE_CONST reverse_iterator<R>& rhs) CASTLE_NOEXCEPT
{
    return !(lhs == rhs);
}

template <typename L, typename R>
bool operator<(CASTLE_CONST reverse_iterator<L>& lhs, CASTLE_CONST reverse_iterator<R>& rhs) CASTLE_NOEXCEPT
{
    return rhs.base() < lhs.base();
}

template <typename L, typename R>
bool operator>(CASTLE_CONST reverse_iterator<L>& lhs, CASTLE_CONST reverse_iterator<R>& rhs) CASTLE_NOEXCEPT
{
    return rhs < lhs;
}

template <typename L, typename R>
bool operator<=(CASTLE_CONST reverse_iterator<L>& lhs, CASTLE_CONST reverse_iterator<R>& rhs) CASTLE_NOEXCEPT
{
    return !(rhs < lhs);
}

template <typename L, typename R>
bool operator>=(CASTLE_CONST reverse_iterator<L>& lhs, CASTLE_CONST reverse_iterator<R>& rhs) CASTLE_NOEXCEPT
{
    return !(lhs < rhs);
}

template <typename Iterator>
reverse_iterator<Iterator> operator+(
    typename reverse_iterator<Iterator>::difference_type offset,
    CASTLE_CONST reverse_iterator<Iterator>& iterator) CASTLE_NOEXCEPT
{
    return iterator + offset;
}

template <typename L, typename R>
difference_type operator-(
    CASTLE_CONST reverse_iterator<L>& lhs,
    CASTLE_CONST reverse_iterator<R>& rhs) CASTLE_NOEXCEPT
{
    return rhs.base() - lhs.base();
}

} // namespace castle

#endif // CASTLE_ITERATOR_REVERSE_ITERATOR_H
