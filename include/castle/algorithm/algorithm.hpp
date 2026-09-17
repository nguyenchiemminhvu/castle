// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @brief STL-free generic algorithms for Castle iterators and raw pointers.
 *
 * Include this header when embedded code needs deterministic search, copy, transform,
 * remove, sort, and ordered-lookup algorithms without `<algorithm>`, heap allocation,
 * exceptions, RTTI, or recursion-heavy implementations.
 *
 * @code
 * #include "castle/algorithm/algorithm.hpp"
 *
 * int values[4] = {4, 1, 3, 2};
 * castle::sort(values, values + 4);
 * int* found = castle::find(values, values + 4, 3);
 * (void)found;
 * @endcode
 */
#ifndef CASTLE_ALGORITHM_ALGORITHM_HPP
#define CASTLE_ALGORITHM_ALGORITHM_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/iterator/iterator.hpp"
#include "castle/utility/move.hpp"
#include "castle/utility/pair.hpp"
#include "castle/utility/swap.hpp"

namespace castle
{

namespace detail
{

/**
 * @brief Checks if an iterator category is compatible with a required category.
 *
 * @tparam ActualCategory The actual iterator category of the iterator.
 * @tparam RequiredCategory The required iterator category to check compatibility against.
 * @return `true` if the actual category is compatible with the required category, `false` otherwise.
 * @note This is typically used internally to enforce iterator category requirements in generic algorithms.
 */
template <typename ActualCategory, typename RequiredCategory>
struct iterator_category_is_compatible
{
private:
    using yes_type = char;
    using no_type = long;

    static yes_type test(RequiredCategory*);
    static no_type test(...);

public:
    static CASTLE_CONSTEXPR bool value = sizeof(test(static_cast<ActualCategory*>(0))) == sizeof(yes_type);
};

/**
 * @brief Checks if an iterator meets a required category.
 *
 * @tparam Iterator The iterator type to check.
 * @tparam RequiredCategory The required iterator category to check against.
 * @return `true` if the iterator meets the required category, `false` otherwise.
 */
template <typename Iterator, typename RequiredCategory>
struct iterator_meets_category
{
    using category = typename iterator_traits<Iterator>::iterator_category;
    static CASTLE_CONSTEXPR bool value = iterator_category_is_compatible<category, RequiredCategory>::value;
};

/**
 * @brief Function object for performing less-than comparisons.
 *
 * @tparam T Type of the left-hand side operand.
 * @tparam U Type of the right-hand side operand.
 * @return `true` if the left-hand side operand is less than the right-hand side operand, `false` otherwise.
 * @note This is typically used as the default comparison function in sorting and searching algorithms.
 */
struct less
{
    template <typename T, typename U>
    CASTLE_CONSTEXPR bool operator()(CASTLE_CONST T& lhs, CASTLE_CONST U& rhs) CASTLE_CONST
    CASTLE_NOEXCEPT(noexcept(lhs < rhs))
    {
        return lhs < rhs;
    }
};

/**
 * @brief Sifts down an element in a heap to maintain the heap property.
 *
 * @tparam TIterator Random access iterator type.
 * @tparam TCompare Comparison function object type.
 * @param first Iterator to the beginning of the heap range.
 * @param root Index of the element to sift down.
 * @param count Number of elements in the heap range.
 * @param compare Comparison function object used to compare elements.
 */
template <typename TIterator, typename TCompare>
void heap_sift_down(TIterator first,
                   difference_type root,
                   difference_type count,
                   TCompare compare) CASTLE_NOEXCEPT(noexcept(compare(*first, *first)))
{
    while (count > 1 && root >= 0 && root <= ((count - 2) / 2)) // LCOV_EXCL_BR_LINE
    {
        difference_type child = (root * 2) + 1;

        if ((child + 1) < count && compare(*(first + child), *(first + child + 1)))
        {
            ++child;
        }

        if (!compare(*(first + root), *(first + child)))
        {
            break;
        }

        castle::swap(*(first + root), *(first + child));
        root = child;
    }
}

}

/**
 * @brief Finds the first element equal to a value.
 *
 * @tparam TInputIterator Input iterator type.
 * @tparam TValue Value type used for comparison.
 * @param first Iterator to the beginning of the range.
 * @param last Iterator one past the end of the range.
 * @param value Value to search for.
 * @return Iterator to the first matching element, or `last` if no match is found.
 */
template <typename TInputIterator, typename TValue>
TInputIterator find(TInputIterator first,
                    TInputIterator last,
                    CASTLE_CONST TValue& value) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TInputIterator, input_iterator_tag>::value,
                  "castle::algorithm: iterator must satisfy InputIterator requirements");

    while (first != last)
    {
        if (*first == value)
        {
            return first;
        }
        ++first;
    }

    return last;
}

/**
 * @brief Finds the first element satisfying a predicate.
 *
 * @tparam TInputIterator Input iterator type.
 * @tparam TUnaryPredicate Predicate type.
 * @param first Iterator to the beginning of the range.
 * @param last Iterator one past the end of the range.
 * @param predicate Unary predicate applied to each element.
 * @return Iterator to the first matching element, or `last` when none match.
 */
template <typename TInputIterator, typename TUnaryPredicate>
TInputIterator find_if(TInputIterator first,
                       TInputIterator last,
                       TUnaryPredicate predicate) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TInputIterator, input_iterator_tag>::value,
                  "castle::algorithm: iterator must satisfy InputIterator requirements");

    while (first != last)
    {
        if (predicate(*first))
        {
            return first;
        }
        ++first;
    }

    return last;
}

/**
 * @brief Finds the first element that does not satisfy a predicate.
 *
 * @tparam TInputIterator Input iterator type.
 * @tparam TUnaryPredicate Predicate type.
 * @param first Iterator to the beginning of the range.
 * @param last Iterator one past the end of the range.
 * @param predicate Unary predicate applied to each element.
 * @return Iterator to the first element for which the predicate returns `false`, or `last`.
 */
template <typename TInputIterator, typename TUnaryPredicate>
TInputIterator find_if_not(TInputIterator first,
                           TInputIterator last,
                           TUnaryPredicate predicate) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TInputIterator, input_iterator_tag>::value,
                  "castle::algorithm: iterator must satisfy InputIterator requirements");

    while (first != last)
    {
        if (!predicate(*first))
        {
            return first;
        }
        ++first;
    }

    return last;
}

/**
 * @brief Counts elements equal to a value.
 *
 * @tparam TInputIterator Input iterator type.
 * @tparam TValue Value type used for comparison.
 * @param first Iterator to the beginning of the range.
 * @param last Iterator one past the end of the range.
 * @param value Value to count.
 * @return Number of matching elements in the range.
 */
template <typename TInputIterator, typename TValue>
size_type count(TInputIterator first,
                TInputIterator last,
                CASTLE_CONST TValue& value) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TInputIterator, input_iterator_tag>::value,
                  "castle::algorithm: iterator must satisfy InputIterator requirements");

    size_type result = 0U;
    while (first != last)
    {
        if (*first == value)
        {
            ++result;
        }
        ++first;
    }

    return result;
}

/**
 * @brief Counts elements satisfying a predicate.
 *
 * @tparam TInputIterator Input iterator type.
 * @tparam TUnaryPredicate Predicate type.
 * @param first Iterator to the beginning of the range.
 * @param last Iterator one past the end of the range.
 * @param predicate Unary predicate applied to each element.
 * @return Number of elements for which the predicate returns `true`.
 */
template <typename TInputIterator, typename TUnaryPredicate>
size_type count_if(TInputIterator first,
                   TInputIterator last,
                   TUnaryPredicate predicate) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TInputIterator, input_iterator_tag>::value,
                  "castle::algorithm: iterator must satisfy InputIterator requirements");

    size_type result = 0U;
    while (first != last)
    {
        if (predicate(*first))
        {
            ++result;
        }
        ++first;
    }

    return result;
}

/**
 * @brief Tests whether every element satisfies a predicate.
 *
 * @tparam TInputIterator Input iterator type.
 * @tparam TUnaryPredicate Predicate type.
 * @param first Iterator to the beginning of the range.
 * @param last Iterator one past the end of the range.
 * @param predicate Unary predicate applied to each element.
 * @return `true` when all elements satisfy the predicate; otherwise `false`.
 */
template <typename TInputIterator, typename TUnaryPredicate>
bool all_of(TInputIterator first,
            TInputIterator last,
            TUnaryPredicate predicate) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TInputIterator, input_iterator_tag>::value,
                  "castle::algorithm: iterator must satisfy InputIterator requirements");

    while (first != last)
    {
        if (!predicate(*first))
        {
            return false;
        }
        ++first;
    }

    return true;
}

/**
 * @brief Tests whether any element satisfies a predicate.
 *
 * @tparam TInputIterator Input iterator type.
 * @tparam TUnaryPredicate Predicate type.
 * @param first Iterator to the beginning of the range.
 * @param last Iterator one past the end of the range.
 * @param predicate Unary predicate applied to each element.
 * @return `true` when at least one element satisfies the predicate; otherwise `false`.
 */
template <typename TInputIterator, typename TUnaryPredicate>
bool any_of(TInputIterator first,
            TInputIterator last,
            TUnaryPredicate predicate) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TInputIterator, input_iterator_tag>::value,
    "castle::algorithm: iterator must satisfy InputIterator requirements");

    while (first != last)
    {
        if (predicate(*first))
        {
            return true;
        }
        ++first;
    }

    return false;
}

/**
 * @brief Tests whether no element satisfies a predicate.
 *
 * @tparam TInputIterator Input iterator type.
 * @tparam TUnaryPredicate Predicate type.
 * @param first Iterator to the beginning of the range.
 * @param last Iterator one past the end of the range.
 * @param predicate Unary predicate applied to each element.
 * @return `true` when the predicate returns `false` for every element; otherwise `false`.
 */
template <typename TInputIterator, typename TUnaryPredicate>
bool none_of(TInputIterator first,
             TInputIterator last,
             TUnaryPredicate predicate) CASTLE_NOEXCEPT
{
    return !any_of(first, last, predicate);
}

/**
 * @brief Compares two ranges for element-wise equality.
 *
 * @tparam TInputIterator1 First input iterator type.
 * @tparam TInputIterator2 Second input iterator type.
 * @param first1 Iterator to the beginning of the first range.
 * @param last1 Iterator one past the end of the first range.
 * @param first2 Iterator to the beginning of the second range.
 * @return `true` when the compared elements are equal; otherwise `false`.
 *
 * @note Additional overloads accept explicit second-range bounds and custom predicates.
 */
template <typename TInputIterator1, typename TInputIterator2>
bool equal(TInputIterator1 first1,
           TInputIterator1 last1,
           TInputIterator2 first2) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TInputIterator1, input_iterator_tag>::value,
                  "castle::algorithm: first iterator must satisfy InputIterator requirements");
    static_assert(detail::iterator_meets_category<TInputIterator2, input_iterator_tag>::value,
                  "castle::algorithm: second iterator must satisfy InputIterator requirements");

    while (first1 != last1)
    {
        if (!(*first1 == *first2))
        {
            return false;
        }
        ++first1;
        ++first2;
    }

    return true;
}

/**
 * @brief Compares two ranges for element-wise equality using a custom predicate.
 *
 * @tparam TBinaryPredicate Binary predicate type.
 * @param first1 Iterator to the beginning of the first range.
 * @param last1 Iterator one past the end of the first range.
 * @param first2 Iterator to the beginning of the second range.
 * @param predicate Binary predicate used to compare elements.
 * @return `true` when the compared elements satisfy the predicate; otherwise `false`.
 */
template <typename TInputIterator1, typename TInputIterator2, typename TBinaryPredicate>
bool equal(TInputIterator1 first1,
           TInputIterator1 last1,
           TInputIterator2 first2,
           TBinaryPredicate predicate) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TInputIterator1, input_iterator_tag>::value,
                  "castle::algorithm: first iterator must satisfy InputIterator requirements");
    static_assert(detail::iterator_meets_category<TInputIterator2, input_iterator_tag>::value,
                  "castle::algorithm: second iterator must satisfy InputIterator requirements");

    while (first1 != last1)
    {
        if (!predicate(*first1, *first2))
        {
            return false;
        }
        ++first1;
        ++first2;
    }

    return true;
}

/**
 * @brief Compares two ranges for element-wise equality.
 *
 * @tparam TInputIterator1 First input iterator type.
 * @tparam TInputIterator2 Second input iterator type.
 * @param first1 Iterator to the beginning of the first range.
 * @param last1 Iterator one past the end of the first range.
 * @param first2 Iterator to the beginning of the second range.
 * @param last2 Iterator one past the end of the second range.
 * @return `true` if the ranges are element-wise equal; otherwise `false`.
 */
template <typename TInputIterator1, typename TInputIterator2>
bool equal(TInputIterator1 first1,
           TInputIterator1 last1,
           TInputIterator2 first2,
           TInputIterator2 last2) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TInputIterator1, input_iterator_tag>::value,
                  "castle::algorithm: first iterator must satisfy InputIterator requirements");
    static_assert(detail::iterator_meets_category<TInputIterator2, input_iterator_tag>::value,
                  "castle::algorithm: second iterator must satisfy InputIterator requirements");

    while (first1 != last1 && first2 != last2)
    {
        if (!(*first1 == *first2))
        {
            return false;
        }
        ++first1;
        ++first2;
    }

    return first1 == last1 && first2 == last2;
}

/**
 * @brief Compares two ranges for element-wise equality using a custom predicate.
 *
 * @tparam TBinaryPredicate Binary predicate type.
 * @param first1 Iterator to the beginning of the first range.
 * @param last1 Iterator one past the end of the first range.
 * @param first2 Iterator to the beginning of the second range.
 * @param last2 Iterator one past the end of the second range.
 * @param predicate Binary predicate that returns `true` if the elements are considered equal.
 * @return `true` if the ranges are element-wise equal according to the predicate; otherwise `false`.
 */
template <typename TInputIterator1, typename TInputIterator2, typename TBinaryPredicate>
bool equal(TInputIterator1 first1,
           TInputIterator1 last1,
           TInputIterator2 first2,
           TInputIterator2 last2,
           TBinaryPredicate predicate) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TInputIterator1, input_iterator_tag>::value,
                  "castle::algorithm: first iterator must satisfy InputIterator requirements");
    static_assert(detail::iterator_meets_category<TInputIterator2, input_iterator_tag>::value,
                  "castle::algorithm: second iterator must satisfy InputIterator requirements");

    while (first1 != last1 && first2 != last2)
    {
        if (!predicate(*first1, *first2))
        {
            return false;
        }
        ++first1;
        ++first2;
    }

    return first1 == last1 && first2 == last2;
}

/**
 * @brief Finds the first position where two ranges differ.
 *
 * @tparam TInputIterator1 First input iterator type.
 * @tparam TInputIterator2 Second input iterator type.
 * @param first1 Iterator to the beginning of the first range.
 * @param last1 Iterator one past the end of the first range.
 * @param first2 Iterator to the beginning of the second range.
 * @return Pair of iterators pointing at the first mismatch in each range.
 *
 * @note Additional overloads accept explicit second-range bounds and custom predicates.
 */
template <typename TInputIterator1, typename TInputIterator2>
pair<TInputIterator1, TInputIterator2> mismatch(TInputIterator1 first1,
                                                  TInputIterator1 last1,
                                                  TInputIterator2 first2) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TInputIterator1, input_iterator_tag>::value,
                  "castle::algorithm: first iterator must satisfy InputIterator requirements");
    static_assert(detail::iterator_meets_category<TInputIterator2, input_iterator_tag>::value,
                  "castle::algorithm: second iterator must satisfy InputIterator requirements");

    while (first1 != last1 && *first1 == *first2)
    {
        ++first1;
        ++first2;
    }

    return pair<TInputIterator1, TInputIterator2>(first1, first2);
}

/**
 * @brief Finds the first position where two ranges differ using a custom predicate.
 *
 * @tparam TInputIterator1 First input iterator type.
 * @tparam TInputIterator2 Second input iterator type.
 * @tparam TBinaryPredicate Binary predicate type.
 * @param first1 Iterator to the beginning of the first range.
 * @param last1 Iterator one past the end of the first range.
 * @param first2 Iterator to the beginning of the second range.
 * @param predicate Binary predicate that returns `true` if the elements are considered equal.
 * @return A pair of iterators pointing to the first position where the two ranges differ.
 */
template <typename TInputIterator1, typename TInputIterator2, typename TBinaryPredicate>
pair<TInputIterator1, TInputIterator2> mismatch(TInputIterator1 first1,
                                                  TInputIterator1 last1,
                                                  TInputIterator2 first2,
                                                  TBinaryPredicate predicate) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TInputIterator1, input_iterator_tag>::value,
                  "castle::algorithm: first iterator must satisfy InputIterator requirements");
    static_assert(detail::iterator_meets_category<TInputIterator2, input_iterator_tag>::value,
                  "castle::algorithm: second iterator must satisfy InputIterator requirements");

    while (first1 != last1 && predicate(*first1, *first2))
    {
        ++first1;
        ++first2;
    }

    return pair<TInputIterator1, TInputIterator2>(first1, first2);
}

/**
 * @brief Finds the first position where two ranges differ.
 *
 * @tparam TInputIterator1 First input iterator type.
 * @tparam TInputIterator2 Second input iterator type.
 * @param first1 Iterator to the beginning of the first range.
 * @param last1 Iterator one past the end of the first range.
 * @param first2 Iterator to the beginning of the second range.
 * @param last2 Iterator one past the end of the second range.
 * @return A pair of iterators pointing to the first position where the two ranges differ.
 */
template <typename TInputIterator1, typename TInputIterator2>
pair<TInputIterator1, TInputIterator2> mismatch(TInputIterator1 first1,
                                                  TInputIterator1 last1,
                                                  TInputIterator2 first2,
                                                  TInputIterator2 last2) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TInputIterator1, input_iterator_tag>::value,
                  "castle::algorithm: first iterator must satisfy InputIterator requirements");
    static_assert(detail::iterator_meets_category<TInputIterator2, input_iterator_tag>::value,
                  "castle::algorithm: second iterator must satisfy InputIterator requirements");

    while (first1 != last1 && first2 != last2 && *first1 == *first2)
    {
        ++first1;
        ++first2;
    }

    return pair<TInputIterator1, TInputIterator2>(first1, first2);
}

/**
 * @brief Finds the first position where two ranges differ, using a binary predicate to compare elements.
 *
 * @tparam TBinaryPredicate Binary predicate type.
 * @param predicate Binary predicate that returns `true` if the elements are considered equal.
 * @return A pair of iterators pointing to the first position where the two ranges differ.
 * @note The binary predicate should define an equivalence relation.
 */
template <typename TInputIterator1, typename TInputIterator2, typename TBinaryPredicate>
pair<TInputIterator1, TInputIterator2> mismatch(TInputIterator1 first1,
                                                  TInputIterator1 last1,
                                                  TInputIterator2 first2,
                                                  TInputIterator2 last2,
                                                  TBinaryPredicate predicate) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TInputIterator1, input_iterator_tag>::value,
                  "castle::algorithm: first iterator must satisfy InputIterator requirements");
    static_assert(detail::iterator_meets_category<TInputIterator2, input_iterator_tag>::value,
                  "castle::algorithm: second iterator must satisfy InputIterator requirements");

    while (first1 != last1 && first2 != last2 && predicate(*first1, *first2))
    {
        ++first1;
        ++first2;
    }

    return pair<TInputIterator1, TInputIterator2>(first1, first2);
}

/**
 * @brief Copies a range into an output iterator.
 *
 * @tparam TInputIterator Input iterator type.
 * @tparam TOutputIterator Output iterator type.
 * @param first Iterator to the beginning of the source range.
 * @param last Iterator one past the end of the source range.
 * @param destination Output iterator receiving copied values.
 * @return Output iterator positioned one past the last copied element.
 */
template <typename TInputIterator, typename TOutputIterator>
TOutputIterator copy(TInputIterator first,
                     TInputIterator last,
                     TOutputIterator destination) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TInputIterator, input_iterator_tag>::value,
                  "castle::algorithm: iterator must satisfy InputIterator requirements");

    while (first != last)
    {
        *destination = *first;
        ++first;
        ++destination;
    }

    return destination;
}

/**
 * @brief Copies a fixed number of elements into an output iterator.
 *
 * @tparam TInputIterator Input iterator type.
 * @tparam TSize Count type.
 * @tparam TOutputIterator Output iterator type.
 * @param first Iterator to the beginning of the source range.
 * @param count_value Number of elements to copy.
 * @param destination Output iterator receiving copied values.
 * @return Output iterator positioned one past the last copied element.
 */
template <typename TInputIterator, typename TSize, typename TOutputIterator>
TOutputIterator copy_n(TInputIterator first,
                       TSize count_value,
                       TOutputIterator destination) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TInputIterator, input_iterator_tag>::value,
                  "castle::algorithm: iterator must satisfy InputIterator requirements");

    while (count_value > 0)
    {
        *destination = *first;
        ++first;
        ++destination;
        --count_value;
    }

    return destination;
}

/**
 * @brief Copies elements satisfying a predicate into an output iterator.
 *
 * @tparam TInputIterator Input iterator type.
 * @tparam TOutputIterator Output iterator type.
 * @tparam TUnaryPredicate Predicate type.
 * @param first Iterator to the beginning of the source range.
 * @param last Iterator one past the end of the source range.
 * @param destination Output iterator receiving copied values.
 * @param predicate Unary predicate applied to each element.
 * @return Output iterator positioned one past the last copied element.
 */
template <typename TInputIterator, typename TOutputIterator, typename TUnaryPredicate>
TOutputIterator copy_if(TInputIterator first,
                        TInputIterator last,
                        TOutputIterator destination,
                        TUnaryPredicate predicate) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TInputIterator, input_iterator_tag>::value,
    "castle::algorithm: iterator must satisfy InputIterator requirements");

    while (first != last)
    {
        if (predicate(*first))
        {
            *destination = *first;
            ++destination;
        }
        ++first;
    }

    return destination;
}

/**
 * @brief Moves a range into an output iterator.
 *
 * @tparam TInputIterator Input iterator type.
 * @tparam TOutputIterator Output iterator type.
 * @param first Iterator to the beginning of the source range.
 * @param last Iterator one past the end of the source range.
 * @param destination Output iterator receiving moved values.
 * @return Output iterator positioned one past the last moved element.
 */
template <typename TInputIterator, typename TOutputIterator>
TOutputIterator move(TInputIterator first,
                     TInputIterator last,
                     TOutputIterator destination) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TInputIterator, input_iterator_tag>::value,
                  "castle::algorithm: iterator must satisfy InputIterator requirements");

    while (first != last)
    {
        *destination = CASTLE_MOVE(*first);
        ++first;
        ++destination;
    }

    return destination;
}

/**
 * @brief Assigns one value to every element in a forward range.
 *
 * @tparam TForwardIterator Forward iterator type.
 * @tparam TValue Assigned value type.
 * @param first Iterator to the beginning of the range.
 * @param last Iterator one past the end of the range.
 * @param value Value assigned to each element.
 */
template <typename TForwardIterator, typename TValue>
void fill(TForwardIterator first,
          TForwardIterator last,
          CASTLE_CONST TValue& value) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TForwardIterator, forward_iterator_tag>::value,
                  "castle::algorithm: iterator must satisfy ForwardIterator requirements");

    while (first != last)
    {
        *first = value;
        ++first;
    }
}

/**
 * @brief Assigns one value to a fixed number of elements.
 *
 * @tparam TForwardIterator Forward iterator type.
 * @tparam TSize Count type.
 * @tparam TValue Assigned value type.
 * @param first Iterator to the first destination element.
 * @param count_value Number of elements to write.
 * @param value Value assigned to each element.
 * @return Iterator one past the last written element.
 */
template <typename TForwardIterator, typename TSize, typename TValue>
TForwardIterator fill_n(TForwardIterator first,
                        TSize count_value,
                        CASTLE_CONST TValue& value) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TForwardIterator, forward_iterator_tag>::value,
                  "castle::algorithm: iterator must satisfy ForwardIterator requirements");

    while (count_value > 0)
    {
        *first = value;
        ++first;
        --count_value;
    }

    return first;
}

/**
 * @brief Applies an operation to input elements and stores the result.
 *
 * @tparam TInputIterator Input iterator type.
 * @tparam TOutputIterator Output iterator type.
 * @tparam TUnaryOperation Unary operation type.
 * @param first Iterator to the beginning of the source range.
 * @param last Iterator one past the end of the source range.
 * @param destination Output iterator receiving transformed values.
 * @param operation Transformation callable.
 * @return Output iterator positioned one past the last written element.
 *
 * @note An additional overload applies a binary operation to two input ranges.
 */
template <typename TInputIterator, typename TOutputIterator, typename TUnaryOperation>
TOutputIterator transform(TInputIterator first,
                          TInputIterator last,
                          TOutputIterator destination,
                          TUnaryOperation operation) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TInputIterator, input_iterator_tag>::value,
                  "castle::algorithm: iterator must satisfy InputIterator requirements");

    while (first != last)
    {
        *destination = operation(*first);
        ++first;
        ++destination;
    }

    return destination;
}

/**
 * @brief Applies a binary operation to corresponding elements of two input ranges and stores the result in an output range.
 *
 * @tparam TInputIterator1 First input iterator type.
 * @tparam TInputIterator2 Second input iterator type.
 * @tparam TOutputIterator Output iterator type.
 * @tparam TBinaryOperation Binary operation type.
 * @param first1 Iterator to the beginning of the first input range.
 * @param last1 Iterator one past the end of the first input range.
 * @param first2 Iterator to the beginning of the second input range.
 * @param destination Output iterator receiving transformed values.
 * @param operation Binary operation callable.
 * @return Output iterator positioned one past the last written element.
 */
template <typename TInputIterator1, typename TInputIterator2, typename TOutputIterator, typename TBinaryOperation>
TOutputIterator transform(TInputIterator1 first1,
                          TInputIterator1 last1,
                          TInputIterator2 first2,
                          TOutputIterator destination,
                          TBinaryOperation operation) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TInputIterator1, input_iterator_tag>::value,
                  "castle::algorithm: first iterator must satisfy InputIterator requirements");
    static_assert(detail::iterator_meets_category<TInputIterator2, input_iterator_tag>::value,
                  "castle::algorithm: second iterator must satisfy InputIterator requirements");

    while (first1 != last1)
    {
        *destination = operation(*first1, *first2);
        ++first1;
        ++first2;
        ++destination;
    }

    return destination;
}

/**
 * @brief Generates values for every element in a forward range.
 *
 * @tparam TForwardIterator Forward iterator type.
 * @tparam TGenerator Generator callable type.
 * @param first Iterator to the beginning of the range.
 * @param last Iterator one past the end of the range.
 * @param generator Callable invoked once per element.
 */
template <typename TForwardIterator, typename TGenerator>
void generate(TForwardIterator first,
              TForwardIterator last,
              TGenerator generator) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TForwardIterator, forward_iterator_tag>::value,
                  "castle::algorithm: iterator must satisfy ForwardIterator requirements");

    while (first != last)
    {
        *first = generator();
        ++first;
    }
}

/**
 * @brief Replaces matching values in a forward range.
 *
 * @tparam TForwardIterator Forward iterator type.
 * @tparam TValue Value type.
 * @param first Iterator to the beginning of the range.
 * @param last Iterator one past the end of the range.
 * @param old_value Value to replace.
 * @param new_value Replacement value.
 */
template <typename TForwardIterator, typename TValue>
void replace(TForwardIterator first,
             TForwardIterator last,
             CASTLE_CONST TValue& old_value,
             CASTLE_CONST TValue& new_value) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TForwardIterator, forward_iterator_tag>::value,
                  "castle::algorithm: iterator must satisfy ForwardIterator requirements");

    while (first != last)
    {
        if (*first == old_value)
        {
            *first = new_value;
        }
        ++first;
    }
}

/**
 * @brief Replaces values that satisfy a predicate.
 *
 * @tparam TForwardIterator Forward iterator type.
 * @tparam TUnaryPredicate Predicate type.
 * @tparam TValue Value type.
 * @param first Iterator to the beginning of the range.
 * @param last Iterator one past the end of the range.
 * @param predicate Predicate selecting elements to replace.
 * @param new_value Replacement value.
 */
template <typename TForwardIterator, typename TUnaryPredicate, typename TValue>
void replace_if(TForwardIterator first,
                TForwardIterator last,
                TUnaryPredicate predicate,
                CASTLE_CONST TValue& new_value) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TForwardIterator, forward_iterator_tag>::value,
                  "castle::algorithm: iterator must satisfy ForwardIterator requirements");

    while (first != last)
    {
        if (predicate(*first))
        {
            *first = new_value;
        }
        ++first;
    }
}

/**
 * @brief Removes matching values by compacting the range in place.
 *
 * @tparam TForwardIterator Forward iterator type.
 * @tparam TValue Value type.
 * @param first Iterator to the beginning of the range.
 * @param last Iterator one past the end of the range.
 * @param value Value to remove.
 * @return Iterator representing the new logical end of the range.
 */
template <typename TForwardIterator, typename TValue>
TForwardIterator remove(TForwardIterator first,
                        TForwardIterator last,
                        CASTLE_CONST TValue& value) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TForwardIterator, forward_iterator_tag>::value,
                  "castle::algorithm: iterator must satisfy ForwardIterator requirements");

    first = find(first, last, value);
    if (first == last)
    {
        return last;
    }

    TForwardIterator result = first;
    ++first;

    while (first != last)
    {
        if (!(*first == value))
        {
            *result = CASTLE_MOVE(*first);
            ++result;
        }
        ++first;
    }

    return result;
}

/**
 * @brief Removes values satisfying a predicate by compacting the range in place.
 *
 * @tparam TForwardIterator Forward iterator type.
 * @tparam TUnaryPredicate Predicate type.
 * @param first Iterator to the beginning of the range.
 * @param last Iterator one past the end of the range.
 * @param predicate Predicate selecting values to remove.
 * @return Iterator representing the new logical end of the range.
 */
template <typename TForwardIterator, typename TUnaryPredicate>
TForwardIterator remove_if(TForwardIterator first,
                           TForwardIterator last,
                           TUnaryPredicate predicate) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TForwardIterator, forward_iterator_tag>::value,
                  "castle::algorithm: iterator must satisfy ForwardIterator requirements");

    first = find_if(first, last, predicate);
    if (first == last)
    {
        return last;
    }

    TForwardIterator result = first;
    ++first;

    while (first != last)
    {
        if (!predicate(*first))
        {
            *result = CASTLE_MOVE(*first);
            ++result;
        }
        ++first;
    }

    return result;
}

/**
 * @brief Removes consecutive duplicates by compacting the range in place.
 *
 * @tparam TForwardIterator Forward iterator type.
 * @param first Iterator to the beginning of the range.
 * @param last Iterator one past the end of the range.
 * @return Iterator representing the new logical end of the deduplicated range.
 *
 * @note An additional overload accepts a binary predicate for custom equivalence.
 */
template <typename TForwardIterator>
TForwardIterator unique(TForwardIterator first,
                        TForwardIterator last) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TForwardIterator, forward_iterator_tag>::value,
                  "castle::algorithm: iterator must satisfy ForwardIterator requirements");

    if (first == last)
    {
        return last;
    }

    TForwardIterator result = first;
    ++first;

    while (first != last)
    {
        if (!(*result == *first))
        {
            ++result;
            *result = CASTLE_MOVE(*first);
        }
        ++first;
    }

    return ++result;
}

/**
 * @brief Removes consecutive duplicates by compacting the range in place, using a binary predicate for custom equivalence.
 *
 * @tparam TForwardIterator Forward iterator type.
 * @tparam TBinaryPredicate Binary predicate type.
 * @param first Iterator to the beginning of the range.
 * @param last Iterator one past the end of the range.
 * @param predicate Binary predicate defining equivalence between elements.
 * @return Iterator representing the new logical end of the deduplicated range.
 */
template <typename TForwardIterator, typename TBinaryPredicate>
TForwardIterator unique(TForwardIterator first,
                        TForwardIterator last,
                        TBinaryPredicate predicate) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TForwardIterator, forward_iterator_tag>::value,
                  "castle::algorithm: iterator must satisfy ForwardIterator requirements");

    if (first == last)
    {
        return last;
    }

    TForwardIterator result = first;
    ++first;

    while (first != last)
    {
        if (!predicate(*result, *first))
        {
            ++result;
            *result = CASTLE_MOVE(*first);
        }
        ++first;
    }

    return ++result;
}

/**
 * @brief Sorts a random-access range in place.
 *
 * @tparam TRandomAccessIterator Random-access iterator type.
 * @tparam TCompare Comparison callable type.
 * @param first Iterator to the beginning of the range.
 * @param last Iterator one past the end of the range.
 * @param compare Callable returning `true` when the first argument should precede the second.
 *
 * @note Castle uses iterative heapsort for deterministic `O(N log N)` worst-case behavior and `O(1)` extra storage.
 */
template <typename TRandomAccessIterator, typename TCompare>
void sort(TRandomAccessIterator first,
          TRandomAccessIterator last,
          TCompare compare) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TRandomAccessIterator, random_access_iterator_tag>::value,
                  "castle::algorithm: iterator must satisfy RandomAccessIterator requirements");

    CASTLE_CONST difference_type count_value = last - first;
    if (count_value <= 1)
    {
        return;
    }


    for (difference_type start = count_value / 2; start > 0; --start)
    {
        detail::heap_sift_down(first, start - 1, count_value, compare);
    }



    for (difference_type end = count_value; end > 1; --end)
    {
        castle::swap(*first, *(first + end - 1));
        detail::heap_sift_down(first, 0, end - 1, compare);
    }
}

/**
 * @brief Sorts a random-access range in place using the default comparison operator (`<`).
 *
 * @tparam TRandomAccessIterator Random-access iterator type.
 * @param first Iterator to the beginning of the range.
 * @param last Iterator one past the end of the range.
 * @note This overload uses the default comparison operator (`<`).
 */
template <typename TRandomAccessIterator>
void sort(TRandomAccessIterator first,
          TRandomAccessIterator last) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TRandomAccessIterator, random_access_iterator_tag>::value,
                  "castle::algorithm: iterator must satisfy RandomAccessIterator requirements");
    sort(first, last, detail::less());
}

/**
 * @brief Finds the first position where a value could be inserted without violating ordering.
 *
 * @tparam TForwardIterator Forward iterator type.
 * @tparam TValue Search value type.
 * @tparam TCompare Comparison callable type.
 * @param first Iterator to the beginning of the ordered range.
 * @param last Iterator one past the end of the ordered range.
 * @param value Value to search for.
 * @param compare Ordering predicate.
 * @return Iterator to the first element that is not ordered before `value`.
 */
template <typename TForwardIterator, typename TValue, typename TCompare>
TForwardIterator lower_bound(TForwardIterator first,
                             TForwardIterator last,
                             CASTLE_CONST TValue& value,
                             TCompare compare) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TForwardIterator, forward_iterator_tag>::value,
                  "castle::algorithm: iterator must satisfy ForwardIterator requirements");

    difference_type length = castle::distance(first, last);

    while (length > 0)
    {
        CASTLE_CONST difference_type half = length / 2;
        TForwardIterator middle = first;
        castle::advance(middle, half);

        if (compare(*middle, value))
        {
            first = middle;
            ++first;
            length -= half + 1;
        }
        else
        {
            last = middle;
            length = half;
        }
    }

    return first;
}

/**
 * @brief Finds the first position where a value could be inserted without violating ordering, using the default comparison operator (`<`).
 *
 * @tparam TForwardIterator Forward iterator type.
 * @tparam TValue Search value type.
 * @param first Iterator to the beginning of the ordered range.
 * @param last Iterator one past the end of the ordered range.
 * @param value Value to search for.
 * @return Iterator to the first element that is not ordered before `value`.
 */
template <typename TForwardIterator, typename TValue>
TForwardIterator lower_bound(TForwardIterator first,
                             TForwardIterator last,
                             CASTLE_CONST TValue& value) CASTLE_NOEXCEPT
{
    return lower_bound(first, last, value, detail::less());
}

/**
 * @brief Finds the first element ordered after a value.
 *
 * @tparam TForwardIterator Forward iterator type.
 * @tparam TValue Search value type.
 * @tparam TCompare Comparison callable type.
 * @param first Iterator to the beginning of the ordered range.
 * @param last Iterator one past the end of the ordered range.
 * @param value Value to search for.
 * @param compare Ordering predicate.
 * @return Iterator to the first element for which `compare(value, element)` is `true`.
 */
template <typename TForwardIterator, typename TValue, typename TCompare>
TForwardIterator upper_bound(TForwardIterator first,
                             TForwardIterator last,
                             CASTLE_CONST TValue& value,
                             TCompare compare) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TForwardIterator, forward_iterator_tag>::value,
                  "castle::algorithm: iterator must satisfy ForwardIterator requirements");

    difference_type length = castle::distance(first, last);

    while (length > 0)
    {
        CASTLE_CONST difference_type half = length / 2;
        TForwardIterator middle = first;
        castle::advance(middle, half);

        if (!compare(value, *middle))
        {
            first = middle;
            ++first;
            length -= half + 1;
        }
        else
        {
            last = middle;
            length = half;
        }
    }

    return first;
}

/**
 * @brief Finds the first element ordered after a value using the default comparison operator (`<`).
 *
 * @tparam TForwardIterator Forward iterator type.
 * @tparam TValue Search value type.
 * @param first Iterator to the beginning of the ordered range.
 * @param last Iterator one past the end of the ordered range.
 * @param value Value to search for.
 * @return Iterator to the first element that is ordered after `value`.
 */
template <typename TForwardIterator, typename TValue>
TForwardIterator upper_bound(TForwardIterator first,
                             TForwardIterator last,
                             CASTLE_CONST TValue& value) CASTLE_NOEXCEPT
{
    return upper_bound(first, last, value, detail::less());
}

/**
 * @brief Tests whether an ordered range contains a value.
 *
 * @tparam TForwardIterator Forward iterator type.
 * @tparam TValue Search value type.
 * @tparam TCompare Comparison callable type.
 * @param first Iterator to the beginning of the ordered range.
 * @param last Iterator one past the end of the ordered range.
 * @param value Value to search for.
 * @param compare Ordering predicate.
 * @return `true` when an equivalent value is present; otherwise `false`.
 */
template <typename TForwardIterator, typename TValue, typename TCompare>
bool binary_search(TForwardIterator first,
                   TForwardIterator last,
                   CASTLE_CONST TValue& value,
                   TCompare compare) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TForwardIterator, forward_iterator_tag>::value,
                  "castle::algorithm: iterator must satisfy ForwardIterator requirements");

    TForwardIterator position = lower_bound(first, last, value, compare);
    return position != last && !compare(value, *position);
}

/**
 * @brief Tests whether an ordered range contains a value using the default comparison operator (`<`).
 *
 * @tparam TForwardIterator Forward iterator type.
 * @tparam TValue Search value type.
 * @param first Iterator to the beginning of the ordered range.
 * @param last Iterator one past the end of the ordered range.
 * @param value Value to search for.
 * @return `true` when an equivalent value is present; otherwise `false`.
 */
template <typename TForwardIterator, typename TValue>
bool binary_search(TForwardIterator first,
                   TForwardIterator last,
                   CASTLE_CONST TValue& value) CASTLE_NOEXCEPT
{
    return binary_search(first, last, value, detail::less());
}

/**
 * @brief Returns the smaller of two values according to a comparator.
 *
 * @tparam T Value type.
 * @tparam TCompare Comparison callable type.
 * @param lhs First value.
 * @param rhs Second value.
 * @param compare Ordering predicate.
 * @return Reference to the smaller value.
 */
template <typename T, typename TCompare>
CASTLE_CONSTEXPR T CASTLE_CONST& min(T CASTLE_CONST& lhs,
                             T CASTLE_CONST& rhs,
                             TCompare compare) CASTLE_NOEXCEPT
{
    return compare(rhs, lhs) ? rhs : lhs;
}

/**
 * @brief Returns the smaller of two values using the default comparison operator (`<`).
 *
 * @tparam T Value type.
 * @param lhs First value.
 * @param rhs Second value.
 * @return Reference to the smaller value.
 */
template <typename T>
CASTLE_CONSTEXPR T CASTLE_CONST& min(T CASTLE_CONST& lhs,
                             T CASTLE_CONST& rhs) CASTLE_NOEXCEPT
{
    return lhs < rhs ? lhs : rhs;
}

/**
 * @brief Returns the larger of two values according to a comparator.
 *
 * @tparam T Value type.
 * @tparam TCompare Comparison callable type.
 * @param lhs First value.
 * @param rhs Second value.
 * @param compare Ordering predicate.
 * @return Reference to the larger value.
 */
template <typename T, typename TCompare>
CASTLE_CONSTEXPR T CASTLE_CONST& max(T CASTLE_CONST& lhs,
                             T CASTLE_CONST& rhs,
                             TCompare compare) CASTLE_NOEXCEPT
{
    return compare(lhs, rhs) ? rhs : lhs;
}

/**
 * @brief Returns the larger of two values using the default comparison operator (`<`).
 *
 * @tparam T Value type.
 * @param lhs First value.
 * @param rhs Second value.
 * @return Reference to the larger value.
 */
template <typename T>
CASTLE_CONSTEXPR T CASTLE_CONST& max(T CASTLE_CONST& lhs,
                             T CASTLE_CONST& rhs) CASTLE_NOEXCEPT
{
    return lhs < rhs ? rhs : lhs;
}

/**
 * @brief Returns the smallest of three values.
 *
 * @tparam T Value type.
 * @param a First value.
 * @param b Second value.
 * @param c Third value.
 * @return Smallest of the three values.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
T min3(T a, T b, T c) CASTLE_NOEXCEPT
{
    return min(min(a, b), c);
}

/**
 * @brief Returns the largest of three values.
 *
 * @tparam T Value type.
 * @param a First value.
 * @param b Second value.
 * @param c Third value.
 * @return Largest of the three values.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
T max3(T a, T b, T c) CASTLE_NOEXCEPT
{
    return max(max(a, b), c);
}

/**
 * @brief Finds the smallest element in a forward range.
 *
 * @tparam TForwardIterator Forward iterator type.
 * @tparam TCompare Comparison callable type.
 * @param first Iterator to the beginning of the range.
 * @param last Iterator one past the end of the range.
 * @param compare Ordering predicate.
 * @return Iterator to the smallest element, or `last` for an empty range.
 */
template <typename TForwardIterator, typename TCompare>
TForwardIterator min_element(TForwardIterator first,
                             TForwardIterator last,
                             TCompare compare) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TForwardIterator, forward_iterator_tag>::value,
                  "castle::algorithm: iterator must satisfy ForwardIterator requirements");

    if (first == last)
    {
        return last;
    }

    TForwardIterator result = first;
    ++first;

    while (first != last)
    {
        if (compare(*first, *result))
        {
            result = first;
        }
        ++first;
    }

    return result;
}

/**
 * @brief Finds the smallest element in a forward range using the default comparison.
 *
 * @tparam TForwardIterator Forward iterator type.
 * @param first Iterator to the beginning of the range.
 * @param last Iterator one past the end of the range.
 * @return Iterator to the smallest element, or `last` for an empty range.
 */
template <typename TForwardIterator>
TForwardIterator min_element(TForwardIterator first,
                             TForwardIterator last) CASTLE_NOEXCEPT
{
    return min_element(first, last, detail::less());
}

/**
 * @brief Finds the largest element in a forward range.
 *
 * @tparam TForwardIterator Forward iterator type.
 * @tparam TCompare Comparison callable type.
 * @param first Iterator to the beginning of the range.
 * @param last Iterator one past the end of the range.
 * @param compare Ordering predicate.
 * @return Iterator to the largest element, or `last` for an empty range.
 */
template <typename TForwardIterator, typename TCompare>
TForwardIterator max_element(TForwardIterator first,
                             TForwardIterator last,
                             TCompare compare) CASTLE_NOEXCEPT
{
    static_assert(detail::iterator_meets_category<TForwardIterator, forward_iterator_tag>::value,
                  "castle::algorithm: iterator must satisfy ForwardIterator requirements");

    if (first == last)
    {
        return last;
    }

    TForwardIterator result = first;
    ++first;

    while (first != last)
    {
        if (compare(*result, *first))
        {
            result = first;
        }
        ++first;
    }

    return result;
}

/**
 * @brief Finds the largest element in a forward range using the default comparison.
 *
 * @tparam TForwardIterator Forward iterator type.
 * @param first Iterator to the beginning of the range.
 * @param last Iterator one past the end of the range.
 * @return Iterator to the largest element, or `last` for an empty range.
 */
template <typename TForwardIterator>
TForwardIterator max_element(TForwardIterator first,
                             TForwardIterator last) CASTLE_NOEXCEPT
{
    return max_element(first, last, detail::less());
}

}

#endif
