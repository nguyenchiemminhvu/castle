#ifndef CASTLE_ITERATOR_FIXED_ITERATOR_H
#define CASTLE_ITERATOR_FIXED_ITERATOR_H

#include "castle/core/compiler.h"
#include "castle/core/types.h"
#include "castle/iterator/traits.h"

namespace castle
{

// Bounded forward/bidirectional iterator wrapper. It never increments beyond
// the supplied last iterator or decrements before the supplied first iterator.
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

    fixed_iterator() CASTLE_NOEXCEPT
        : current_(), first_(), last_()
    {
    }

    fixed_iterator(Iterator first, Iterator current, Iterator last) CASTLE_NOEXCEPT
        : current_(current), first_(first), last_(last)
    {
    }

    Iterator base() CASTLE_CONST CASTLE_NOEXCEPT { return current_; }
    bool valid() CASTLE_CONST CASTLE_NOEXCEPT { return current_ != last_; }

    reference operator*() CASTLE_CONST CASTLE_NOEXCEPT { return *current_; }
    pointer operator->() CASTLE_CONST CASTLE_NOEXCEPT { return current_; }

    fixed_iterator& operator++() CASTLE_NOEXCEPT
    {
        if (current_ != last_)
        {
            ++current_;
        }
        return *this;
    }

    fixed_iterator operator++(int) CASTLE_NOEXCEPT
    {
        fixed_iterator temp(*this);
        ++(*this);
        return temp;
    }

    fixed_iterator& operator--() CASTLE_NOEXCEPT
    {
        if (current_ != first_)
        {
            --current_;
        }
        return *this;
    }

    fixed_iterator operator--(int) CASTLE_NOEXCEPT
    {
        fixed_iterator temp(*this);
        --(*this);
        return temp;
    }

    bool operator==(CASTLE_CONST fixed_iterator& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return current_ == other.current_;
    }

    bool operator!=(CASTLE_CONST fixed_iterator& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return !(*this == other);
    }

private:
    Iterator current_;
    Iterator first_;
    Iterator last_;
};

} // namespace castle

#endif // CASTLE_ITERATOR_FIXED_ITERATOR_H
