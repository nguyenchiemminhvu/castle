// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @brief STL-independent iterator algorithms for Castle iterators and pointers.
 *
 * Use these helpers when generic embedded code needs `distance`, `advance`, `next`, or
 * `prev` semantics without depending on `<iterator>`. The implementation dispatches on
 * Castle iterator category tags so random-access iterators stay constant time while input,
 * forward, and bidirectional iterators use deterministic loops.
 *
 * @code
 * uint8_t values[5U] = {1U, 2U, 3U, 4U, 5U};
 * auto third = castle::next(values, 2);
 * auto distance = castle::distance(values, values + 5U);
 * (void)third;
 * (void)distance;
 * @endcode
 */
#ifndef CASTLE_ITERATOR_OPERATIONS_HPP
#define CASTLE_ITERATOR_OPERATIONS_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/iterator/traits.hpp"

namespace castle
{
namespace detail
{

/**
 * @brief Implementation of `distance` for random-access iterators.
 * @tparam Iterator Random-access iterator or pointer type.
 * @param first Start of the range.
 * @param last End of the range.
 * @return Signed distance from `first` to `last`.
 */
template <typename Iterator>
difference_type distance_impl(Iterator first, Iterator last, random_access_iterator_tag) CASTLE_NOEXCEPT
{
    return last - first;
}

/**
 * @brief Implementation of `distance` for input iterators.
 * @tparam Iterator Input iterator or pointer type.
 * @param first Start of the range.
 * @param last End of the range.
 * @return Signed distance from `first` to `last`.
 */
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

/**
 * @brief Implementation of `advance` for random-access iterators.
 * @tparam Iterator Random-access iterator or pointer type.
 * @param iterator Iterator to modify in place.
 * @param offset Signed step count to apply.
 * @note Random-access iterators use constant-time arithmetic.
 */
template <typename Iterator>
void advance_impl(Iterator& iterator, difference_type offset, random_access_iterator_tag) CASTLE_NOEXCEPT
{
    iterator += offset;
}

/**
 * @brief Implementation of `advance` for bidirectional iterators.
 * @tparam Iterator Bidirectional iterator or pointer type.
 * @param iterator Iterator to modify in place.
 * @param offset Signed step count to apply.
 * @note Bidirectional iterators move forward or backward linearly.
 */
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

/**
 * @brief Implementation of `advance` for input iterators.
 * @tparam Iterator Input iterator or pointer type.
 * @param iterator Iterator to modify in place.
 * @param offset Signed step count to apply.
 * @note Input iterators only support forward progress linearly.
 */
template <typename Iterator>
void advance_impl(Iterator& iterator, difference_type offset, input_iterator_tag) CASTLE_NOEXCEPT
{
    // Input and forward iterators only support forward progress in this overload.
    while (offset > 0)
    {
        ++iterator;
        --offset;
    }
}

}

/**
 * @brief Computes the number of increments needed to reach `last` from `first`.
 * @tparam Iterator Iterator or pointer type.
 * @param first Start of the range.
 * @param last End of the range.
 * @return Signed distance from `first` to `last`.
 * @note Random-access iterators use constant-time subtraction.
 * @note Input, forward, and bidirectional iterators are advanced linearly until `last`.
 */
template <typename Iterator>
difference_type distance(Iterator first, Iterator last) CASTLE_NOEXCEPT
{
    using category = typename iterator_traits<Iterator>::iterator_category;
    return detail::distance_impl(first, last, category{});
}

/**
 * @brief Moves an iterator by a signed offset.
 * @tparam Iterator Iterator or pointer type.
 * @param iterator Iterator to modify in place.
 * @param offset Signed step count to apply.
 * @note Random-access iterators use constant-time arithmetic.
 * @note Bidirectional iterators move forward or backward linearly.
 * @warning Input and forward iterators ignore negative offsets because only forward
 *          advancement is available through their category dispatch.
 */
template <typename Iterator>
void advance(Iterator& iterator, difference_type offset) CASTLE_NOEXCEPT
{
    using category = typename iterator_traits<Iterator>::iterator_category;
    detail::advance_impl(iterator, offset, category{});
}

/**
 * @brief Returns a copy of an iterator advanced by a signed offset.
 * @tparam Iterator Iterator or pointer type.
 * @param iterator Iterator to copy and advance.
 * @param offset Signed step count to apply; defaults to `1`.
 * @return Advanced iterator copy.
 * @note The original iterator is left unchanged.
 * @warning Negative offsets are only effective for bidirectional or random-access iterators.
 */
template <typename Iterator>
Iterator next(Iterator iterator, difference_type offset = 1) CASTLE_NOEXCEPT
{
    advance(iterator, offset);
    return iterator;
}

/**
 * @brief Returns a copy of an iterator moved backward by a signed offset.
 * @tparam Iterator Iterator or pointer type.
 * @param iterator Iterator to copy and move.
 * @param offset Positive step count to move backward; defaults to `1`.
 * @return Iterator copy after backward movement.
 * @note The original iterator is left unchanged.
 * @warning Meaningful backward movement requires a bidirectional or random-access iterator.
 *          Input and forward iterators remain unchanged because negative offsets are ignored.
 */
template <typename Iterator>
Iterator prev(Iterator iterator, difference_type offset = 1) CASTLE_NOEXCEPT
{
    advance(iterator, -offset);
    return iterator;
}

}

#endif
