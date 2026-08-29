#pragma once

#include "castle/iterators/iterator_traits.h"
#include "castle/iterators/reverse_iterator.h"

#include <cassert>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

// Debug-only contract check for iterator preconditions (same-container ordering,
// non-null dereference). Compiles out under NDEBUG. Redefine before including
// this header to route violations to a custom handler.
#ifndef CASTLE_ITERATOR_ASSERT
    #define CASTLE_ITERATOR_ASSERT(cond, msg) assert((cond) && (msg))
#endif

// ─────────────────────────────────────────────────────────────────────────────
// basic_iterator — a logical cursor over a Castle container.
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Unlike an STL contiguous iterator, a Castle iterator does NOT store a T*.
 * A ring_buffer (and later a heap_buffer) has a physical storage layout that
 * differs from its logical traversal order, so `++it` must not be `++T*`.
 *
 * Instead the iterator stores:
 *
 *     Container*      container_   — the owning container
 *     difference_type position_    — a logical index into that container
 *
 * Turning a logical position into an element is delegated to a *traversal
 * policy*. This keeps iterator mechanics fully separated from data-structure
 * traversal: the same basic_iterator works for contiguous strings, circular
 * buffers, stacks and heaps — each container only supplies the mapping.
 *
 *                     basic_iterator
 *                           |
 *              +------------+------------+
 *              |                         |
 *        traversal policy         iterator category
 *              |                         |
 *        iterator_at_traversal     forward / random_access / ...
 *
 * Iterator invalidation is defined by the owning container. Contract:
 *   • Iterator lifetime must not exceed the owning container's lifetime.
 *   • Any mutating container operation invalidates existing iterators.
 *   • Dereferencing end() (or a singular iterator) is undefined behaviour.
 *   • Ordering (<, >, ...) and distance (a - b) require both iterators to
 *     refer to the same container; violations trip CASTLE_ITERATOR_ASSERT in
 *     debug builds.
 */

namespace castle
{
namespace iterators
{

// ── traversal policy ─────────────────────────────────────────────────────────

/**
 * The default policy maps a logical index onto an element through the
 * container's `iterator_at(index)` member.
 *
 * Contract for iterator_at(i):
 *     • precondition : 0 <= i < container.size()
 *     • complexity   : O(1)
 *     • returns      : reference (or proxy reference) to logical element i
 *
 * A container whose logical order differs from its storage order (ring_buffer,
 * heaps, ...) implements that one function; basic_iterator itself never needs
 * to know the layout. A future policy could impose a different traversal order
 * (e.g. heap-sorted) without changing basic_iterator.
 */
struct iterator_at_traversal
{
    template <typename Container>
    static constexpr decltype(auto) get(Container& container, std::size_t index) noexcept
    {
        return container.iterator_at(index);
    }
};

// Backwards-compatible spelling of the default traversal policy.
using container_traversal = iterator_at_traversal;

namespace detail
{
// Holds the dereferenced value so that `it->member` still works when a
// traversal yields a proxy reference (e.g. a future bit_buffer) instead of a
// real T&. For genuine lvalue references basic_iterator returns a plain pointer
// and never instantiates this.
template <typename Reference>
struct arrow_proxy
{
    Reference value;
    constexpr Reference* operator->() noexcept { return std::addressof(value); }
};
} // namespace detail

// Enables a member overload only when the iterator category is at least the
// required capability (e.g. random-access affordances on a random-access
// iterator). Kept at namespace scope so it is visible in the default template
// arguments of the member declarations below.
template <typename Required, typename Actual>
using enable_if_at_least =
    typename std::enable_if<std::is_base_of<Required, Actual>::value, int>::type;

// ── basic_iterator ───────────────────────────────────────────────────────────

template <
    typename Container,
    typename Traversal = iterator_at_traversal,
    typename Category  = random_access_iterator_tag>
class basic_iterator
{
private:
    template <typename, typename, typename>
    friend class basic_iterator;

    using container_reference =
        typename std::conditional<
            std::is_const<Container>::value,
            const Container&,
            Container&>::type;

    using dereference_type =
        decltype(Traversal::get(std::declval<container_reference>(), std::size_t{}));

public:
    using iterator_category = Category;
    using value_type =
        typename std::remove_cv<
            typename std::remove_reference<dereference_type>::type>::type;
    using difference_type = std::ptrdiff_t;
    using reference       = dereference_type;
    // A real lvalue reference yields a real pointer; a proxy reference yields a
    // small holder so operator-> never dangles on a temporary.
    using pointer =
        typename std::conditional<
            std::is_reference<dereference_type>::value,
            typename std::add_pointer<
                typename std::remove_reference<reference>::type>::type,
            detail::arrow_proxy<reference>>::type;

    constexpr basic_iterator() noexcept
        : container_(nullptr)
        , position_(0)
    {
    }

    constexpr basic_iterator(Container* container, difference_type position) noexcept
        : container_(container)
        , position_(position)
    {
    }

    // Only the safe iterator -> const_iterator direction is permitted.
    template <
        typename OtherContainer,
        typename OtherTraversal,
        typename OtherCategory,
        typename std::enable_if<
            std::is_same<Traversal, OtherTraversal>::value &&
            std::is_same<Category, OtherCategory>::value &&
            std::is_convertible<OtherContainer*, Container*>::value,
            int>::type = 0>
    constexpr basic_iterator(
        const basic_iterator<OtherContainer, OtherTraversal, OtherCategory>& other) noexcept
        : container_(other.container_)
        , position_(other.position_)
    {
    }

    // ── observers ─────────────────────────────────────────────────────────────

    constexpr Container*     container() const noexcept { return container_; }
    constexpr difference_type position() const noexcept { return position_; }

    reference operator*() const noexcept
    {
        CASTLE_ITERATOR_ASSERT(container_ != nullptr,
                               "dereferencing a singular castle iterator");
        return Traversal::get(*container_, static_cast<std::size_t>(position_));
    }

    pointer operator->() const noexcept
    {
        if constexpr (std::is_reference<dereference_type>::value)
        {
            return std::addressof(operator*());
        }
        else
        {
            return pointer{ operator*() };
        }
    }

    // ── increment (all categories) ────────────────────────────────────────────

    basic_iterator& operator++() noexcept { ++position_; return *this; }

    basic_iterator operator++(int) noexcept
    {
        basic_iterator copy(*this);
        ++(*this);
        return copy;
    }

    // ── decrement (bidirectional and above) ───────────────────────────────────

    template <typename C = Category, enable_if_at_least<bidirectional_iterator_tag, C> = 0>
    basic_iterator& operator--() noexcept { --position_; return *this; }

    template <typename C = Category, enable_if_at_least<bidirectional_iterator_tag, C> = 0>
    basic_iterator operator--(int) noexcept
    {
        basic_iterator copy(*this);
        --(*this);
        return copy;
    }

    // ── random-access movement ────────────────────────────────────────────────

    template <typename C = Category, enable_if_at_least<random_access_iterator_tag, C> = 0>
    basic_iterator& operator+=(difference_type n) noexcept { position_ += n; return *this; }

    template <typename C = Category, enable_if_at_least<random_access_iterator_tag, C> = 0>
    basic_iterator& operator-=(difference_type n) noexcept { position_ -= n; return *this; }

    template <typename C = Category, enable_if_at_least<random_access_iterator_tag, C> = 0>
    basic_iterator operator+(difference_type n) const noexcept
    {
        basic_iterator result(*this);
        result += n;
        return result;
    }

    template <typename C = Category, enable_if_at_least<random_access_iterator_tag, C> = 0>
    basic_iterator operator-(difference_type n) const noexcept
    {
        basic_iterator result(*this);
        result -= n;
        return result;
    }

    template <typename C = Category, enable_if_at_least<random_access_iterator_tag, C> = 0>
    reference operator[](difference_type n) const noexcept { return *(*this + n); }

    // ── same-type comparisons ─────────────────────────────────────────────────

    constexpr bool operator==(const basic_iterator& other) const noexcept
    {
        return container_ == other.container_ && position_ == other.position_;
    }

    constexpr bool operator!=(const basic_iterator& other) const noexcept
    {
        return !(*this == other);
    }

    template <typename C = Category, enable_if_at_least<random_access_iterator_tag, C> = 0>
    constexpr bool operator<(const basic_iterator& other) const noexcept
    {
        CASTLE_ITERATOR_ASSERT(container_ == other.container_,
                               "castle iterator ordering requires the same container");
        return position_ < other.position_;
    }

    template <typename C = Category, enable_if_at_least<random_access_iterator_tag, C> = 0>
    constexpr bool operator>(const basic_iterator& other) const noexcept { return other < *this; }

    template <typename C = Category, enable_if_at_least<random_access_iterator_tag, C> = 0>
    constexpr bool operator<=(const basic_iterator& other) const noexcept { return !(other < *this); }

    template <typename C = Category, enable_if_at_least<random_access_iterator_tag, C> = 0>
    constexpr bool operator>=(const basic_iterator& other) const noexcept { return !(*this < other); }

    template <typename C = Category, enable_if_at_least<random_access_iterator_tag, C> = 0>
    constexpr difference_type operator-(const basic_iterator& other) const noexcept
    {
        CASTLE_ITERATOR_ASSERT(container_ == other.container_,
                               "castle iterator difference requires the same container");
        return position_ - other.position_;
    }

private:
    Container*      container_;
    difference_type position_;
};

// ── cross const/non-const comparisons ────────────────────────────────────────

template <typename T, typename U>
using enable_if_same_ignoring_const =
    typename std::enable_if<
        std::is_same<
            typename std::remove_const<T>::type,
            typename std::remove_const<U>::type>::value,
        int>::type;

template <typename T, typename U, typename Category>
using enable_if_same_random_access =
    typename std::enable_if<
        std::is_same<
            typename std::remove_const<T>::type,
            typename std::remove_const<U>::type>::value &&
        std::is_base_of<random_access_iterator_tag, Category>::value,
        int>::type;

template <typename L, typename R, typename Tr, typename Cat,
          enable_if_same_ignoring_const<L, R> = 0>
constexpr bool operator==(const basic_iterator<L, Tr, Cat>& lhs,
                          const basic_iterator<R, Tr, Cat>& rhs) noexcept
{
    return lhs.container() == rhs.container() && lhs.position() == rhs.position();
}

template <typename L, typename R, typename Tr, typename Cat,
          enable_if_same_ignoring_const<L, R> = 0>
constexpr bool operator!=(const basic_iterator<L, Tr, Cat>& lhs,
                          const basic_iterator<R, Tr, Cat>& rhs) noexcept
{
    return !(lhs == rhs);
}

template <typename L, typename R, typename Tr, typename Cat,
          enable_if_same_random_access<L, R, Cat> = 0>
constexpr bool operator<(const basic_iterator<L, Tr, Cat>& lhs,
                         const basic_iterator<R, Tr, Cat>& rhs) noexcept
{
    CASTLE_ITERATOR_ASSERT(lhs.container() == rhs.container(),
                           "castle iterator ordering requires the same container");
    return lhs.position() < rhs.position();
}

template <typename L, typename R, typename Tr, typename Cat,
          enable_if_same_random_access<L, R, Cat> = 0>
constexpr bool operator>(const basic_iterator<L, Tr, Cat>& lhs,
                         const basic_iterator<R, Tr, Cat>& rhs) noexcept
{
    return rhs < lhs;
}

template <typename L, typename R, typename Tr, typename Cat,
          enable_if_same_random_access<L, R, Cat> = 0>
constexpr bool operator<=(const basic_iterator<L, Tr, Cat>& lhs,
                          const basic_iterator<R, Tr, Cat>& rhs) noexcept
{
    return !(rhs < lhs);
}

template <typename L, typename R, typename Tr, typename Cat,
          enable_if_same_random_access<L, R, Cat> = 0>
constexpr bool operator>=(const basic_iterator<L, Tr, Cat>& lhs,
                          const basic_iterator<R, Tr, Cat>& rhs) noexcept
{
    return !(lhs < rhs);
}

template <typename L, typename R, typename Tr, typename Cat,
          enable_if_same_random_access<L, R, Cat> = 0>
constexpr typename basic_iterator<L, Tr, Cat>::difference_type operator-(
    const basic_iterator<L, Tr, Cat>& lhs,
    const basic_iterator<R, Tr, Cat>& rhs) noexcept
{
    CASTLE_ITERATOR_ASSERT(lhs.container() == rhs.container(),
                           "castle iterator difference requires the same container");
    return lhs.position() - rhs.position();
}

// n + iterator
template <typename Container, typename Tr, typename Cat,
          typename std::enable_if<
              std::is_base_of<random_access_iterator_tag, Cat>::value, int>::type = 0>
basic_iterator<Container, Tr, Cat> operator+(
    typename basic_iterator<Container, Tr, Cat>::difference_type n,
    const basic_iterator<Container, Tr, Cat>& it) noexcept
{
    return it + n;
}

// ── generic iterator algorithms ──────────────────────────────────────────────

template <typename Iterator>
constexpr void advance(
    Iterator& it,
    typename iterator_traits<Iterator>::difference_type n) noexcept
{
    using category = typename iterator_traits<Iterator>::iterator_category;

    if constexpr (std::is_base_of<random_access_iterator_tag, category>::value)
    {
        it += n;
    }
    else if constexpr (std::is_base_of<bidirectional_iterator_tag, category>::value)
    {
        for (; n > 0; --n) { ++it; }
        for (; n < 0; ++n) { --it; }
    }
    else
    {
        for (; n > 0; --n) { ++it; }
    }
}

template <typename Iterator>
constexpr Iterator next(
    Iterator it,
    typename iterator_traits<Iterator>::difference_type n = 1) noexcept
{
    // Qualified: the std category tag makes std an associated namespace, so an
    // unqualified advance() would be ambiguous with std::advance.
    castle::iterators::advance(it, n);
    return it;
}

template <typename Iterator>
constexpr Iterator prev(
    Iterator it,
    typename iterator_traits<Iterator>::difference_type n = 1) noexcept
{
    castle::iterators::advance(it, -n);
    return it;
}

template <typename Iterator>
constexpr typename iterator_traits<Iterator>::difference_type distance(
    Iterator first,
    Iterator last) noexcept
{
    using category = typename iterator_traits<Iterator>::iterator_category;

    if constexpr (std::is_base_of<random_access_iterator_tag, category>::value)
    {
        return last - first;
    }
    else
    {
        typename iterator_traits<Iterator>::difference_type result = 0;
        for (; first != last; ++first) { ++result; }
        return result;
    }
}

} // namespace iterators

// The algorithm primitives read naturally at the top level, e.g.
//     castle::distance(q.begin(), q.end())
//     castle::next(q.begin(), 2)
using iterators::advance;
using iterators::distance;
using iterators::next;
using iterators::prev;

} // namespace castle
