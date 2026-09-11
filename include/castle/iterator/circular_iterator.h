#ifndef CASTLE_ITERATOR_CIRCULAR_ITERATOR_H
#define CASTLE_ITERATOR_CIRCULAR_ITERATOR_H

#include "castle/core/compiler.h"
#include "castle/iterator/traits.h"

namespace castle
{

// Circular iterator over a half-open range [first, last). Empty ranges are
// invalid and should not be dereferenced.
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

    circular_iterator() CASTLE_NOEXCEPT
        : current_(), first_(), last_()
    {
    }

    circular_iterator(Iterator first, Iterator last, Iterator current) CASTLE_NOEXCEPT
        : current_(current), first_(first), last_(last)
    {
    }

    Iterator base() CASTLE_CONST CASTLE_NOEXCEPT { return current_; }

    reference operator*() CASTLE_CONST CASTLE_NOEXCEPT { return *current_; }
    pointer operator->() CASTLE_CONST CASTLE_NOEXCEPT { return current_; }

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

    circular_iterator operator++(int) CASTLE_NOEXCEPT
    {
        circular_iterator temp(*this);
        ++(*this);
        return temp;
    }

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

    circular_iterator operator--(int) CASTLE_NOEXCEPT
    {
        circular_iterator temp(*this);
        --(*this);
        return temp;
    }

    bool operator==(CASTLE_CONST circular_iterator& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return current_ == other.current_;
    }

    bool operator!=(CASTLE_CONST circular_iterator& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return !(*this == other);
    }

private:
    Iterator current_;
    Iterator first_;
    Iterator last_;
};

} // namespace castle

#endif // CASTLE_ITERATOR_CIRCULAR_ITERATOR_H
