#ifndef CASTLE_ITERATOR_OPERATIONS_H
#define CASTLE_ITERATOR_OPERATIONS_H

#include "castle/core/compiler.h"
#include "castle/core/types.h"
#include "castle/iterator/traits.h"

namespace castle
{
namespace detail
{

template <typename Iterator>
difference_type distance_impl(Iterator first, Iterator last, random_access_iterator_tag) CASTLE_NOEXCEPT
{
    return last - first;
}

template <typename Iterator>
difference_type distance_impl(Iterator first, Iterator last, input_iterator_tag) CASTLE_NOEXCEPT
{
    difference_type distance = 0;
    while (first != last)
    {
        ++first;
        ++distance;
    }
    return distance;
}

template <typename Iterator>
void advance_impl(Iterator& iterator, difference_type offset, random_access_iterator_tag) CASTLE_NOEXCEPT
{
    iterator += offset;
}

template <typename Iterator>
void advance_impl(Iterator& iterator, difference_type offset, bidirectional_iterator_tag) CASTLE_NOEXCEPT
{
    if (offset >= 0)
    {
        while (offset > 0)
        {
            ++iterator;
            --offset;
        }
    }
    else
    {
        while (offset < 0)
        {
            --iterator;
            ++offset;
        }
    }
}

template <typename Iterator>
void advance_impl(Iterator& iterator, difference_type offset, input_iterator_tag) CASTLE_NOEXCEPT
{
    // Input/forward iterators can only be advanced forward.
    while (offset > 0)
    {
        ++iterator;
        --offset;
    }
}

} // namespace detail

template <typename Iterator>
difference_type distance(Iterator first, Iterator last) CASTLE_NOEXCEPT
{
    using category = typename iterator_traits<Iterator>::iterator_category;
    return detail::distance_impl(first, last, category{});
}

template <typename Iterator>
void advance(Iterator& iterator, difference_type offset) CASTLE_NOEXCEPT
{
    using category = typename iterator_traits<Iterator>::iterator_category;
    detail::advance_impl(iterator, offset, category{});
}

template <typename Iterator>
Iterator next(Iterator iterator, difference_type offset = 1) CASTLE_NOEXCEPT
{
    advance(iterator, offset);
    return iterator;
}

template <typename Iterator>
Iterator prev(Iterator iterator, difference_type offset = 1) CASTLE_NOEXCEPT
{
    advance(iterator, -offset);
    return iterator;
}

} // namespace castle

#endif // CASTLE_ITERATOR_OPERATIONS_H
