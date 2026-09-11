#ifndef CASTLE_ALGORITHM_ALGORITHM_H
#define CASTLE_ALGORITHM_ALGORITHM_H

#include "castle/core/compiler.h"
#include "castle/core/types.h"
#include "castle/iterator/iterator.h"
#include "castle/utility/move.h"
#include "castle/utility/pair.h"
#include "castle/utility/swap.h"

namespace castle
{

namespace detail
{

// Category compatibility is expressed through the iterator-tag inheritance
// hierarchy already used by Castle.  A random-access iterator, for example,
// is convertible to a forward_iterator_tag pointer because its tag derives
// from forward_iterator_tag.
template <typename ActualCategory, typename RequiredCategory>
struct iterator_category_is_compatible
{
private:
using yes_type = char;
using no_type = long;

static yes_type test(RequiredCategory*);
static no_type test(...);

public:
static constexpr bool value = sizeof(test(static_cast<ActualCategory*>(0))) == sizeof(yes_type);
};

template <typename Iterator, typename RequiredCategory>
struct iterator_meets_category
{
using category = typename iterator_traits<Iterator>::iterator_category;
static constexpr bool value = iterator_category_is_compatible<category, RequiredCategory>::value;
};

struct less
{
template <typename T, typename U>
CASTLE_CONSTEXPR bool operator()(CASTLE_CONST T& lhs, CASTLE_CONST U& rhs) const
CASTLE_NOEXCEPT(noexcept(lhs < rhs))
{
return lhs < rhs;
}
};

template <typename TIterator, typename TCompare>
void heap_sift_down(TIterator first,
                   difference_type root,
                   difference_type count,
                   TCompare compare) CASTLE_NOEXCEPT(noexcept(compare(*first, *first)))
{
// `compare(a, b)` means "a is ordered before b".  The heap is therefore a
// max-heap for the default less-than comparator, which produces ascending
// output after repeated extraction.
while (count > 1 && root >= 0 && root <= ((count - 2) / 2))
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

} // namespace detail

// ============================================================================
// Non-modifying sequence operations
// ============================================================================

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

template <typename TInputIterator, typename TUnaryPredicate>
bool none_of(TInputIterator first,
             TInputIterator last,
             TUnaryPredicate predicate) CASTLE_NOEXCEPT
{
return !any_of(first, last, predicate);
}

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

// ============================================================================
// Modifying sequence operations
// ============================================================================

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

// ============================================================================
// Sorting and searching
// ============================================================================

template <typename TRandomAccessIterator, typename TCompare>
void sort(TRandomAccessIterator first,
          TRandomAccessIterator last,
          TCompare compare) CASTLE_NOEXCEPT
{
static_assert(detail::iterator_meets_category<TRandomAccessIterator, random_access_iterator_tag>::value,
"castle::algorithm: iterator must satisfy RandomAccessIterator requirements");

const difference_type count_value = last - first;
if (count_value <= 1)
{
return;
}

// Build a max-heap.
for (difference_type start = count_value / 2; start > 0; --start)
{
detail::heap_sift_down(first, start - 1, count_value, compare);
}

// Repeatedly move the largest item to the end.  No recursion and no dynamic
// storage are used; the implementation has deterministic O(n log n) bounds.
for (difference_type end = count_value; end > 1; --end)
{
castle::swap(*first, *(first + end - 1));
detail::heap_sift_down(first, 0, end - 1, compare);
}
}

template <typename TRandomAccessIterator>
void sort(TRandomAccessIterator first,
          TRandomAccessIterator last) CASTLE_NOEXCEPT
{
static_assert(detail::iterator_meets_category<TRandomAccessIterator, random_access_iterator_tag>::value,
"castle::algorithm: iterator must satisfy RandomAccessIterator requirements");
sort(first, last, detail::less());
}

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
const difference_type half = length / 2;
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

template <typename TForwardIterator, typename TValue>
TForwardIterator lower_bound(TForwardIterator first,
                             TForwardIterator last,
                             CASTLE_CONST TValue& value) CASTLE_NOEXCEPT
{
return lower_bound(first, last, value, detail::less());
}

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
const difference_type half = length / 2;
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

template <typename TForwardIterator, typename TValue>
TForwardIterator upper_bound(TForwardIterator first,
                             TForwardIterator last,
                             CASTLE_CONST TValue& value) CASTLE_NOEXCEPT
{
return upper_bound(first, last, value, detail::less());
}

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

template <typename TForwardIterator, typename TValue>
bool binary_search(TForwardIterator first,
                   TForwardIterator last,
                   CASTLE_CONST TValue& value) CASTLE_NOEXCEPT
{
return binary_search(first, last, value, detail::less());
}

// ============================================================================
// Min / max operations
// ============================================================================

template <typename T, typename TCompare>
CASTLE_CONSTEXPR T const& min(T const& lhs,
                             T const& rhs,
                             TCompare compare) CASTLE_NOEXCEPT
{
return compare(rhs, lhs) ? rhs : lhs;
}

template <typename T>
CASTLE_CONSTEXPR T const& min(T const& lhs,
                             T const& rhs) CASTLE_NOEXCEPT
{
return lhs < rhs ? lhs : rhs;
}

template <typename T, typename TCompare>
CASTLE_CONSTEXPR T const& max(T const& lhs,
                             T const& rhs,
                             TCompare compare) CASTLE_NOEXCEPT
{
return compare(lhs, rhs) ? rhs : lhs;
}

template <typename T>
CASTLE_CONSTEXPR T const& max(T const& lhs,
                             T const& rhs) CASTLE_NOEXCEPT
{
return lhs < rhs ? rhs : lhs;
}

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

template <typename TForwardIterator>
TForwardIterator min_element(TForwardIterator first,
                             TForwardIterator last) CASTLE_NOEXCEPT
{
return min_element(first, last, detail::less());
}

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

template <typename TForwardIterator>
TForwardIterator max_element(TForwardIterator first,
                             TForwardIterator last) CASTLE_NOEXCEPT
{
return max_element(first, last, detail::less());
}

} // namespace castle

#endif // CASTLE_ALGORITHM_ALGORITHM_H
