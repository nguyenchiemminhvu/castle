#ifndef CASTLE_CONTAINER_INITIALIZER_LIST_HPP
#define CASTLE_CONTAINER_INITIALIZER_LIST_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/types.hpp"
#include "castle/core/traits.hpp"
#include "castle/utility/forward.hpp"

#include "castle/iterator/traits.hpp"
#include "castle/iterator/reverse_iterator.hpp"

// ============================================================================
// castle/container/initializer_list.h
// ----------------------------------------------------------------------------
// `{ ... }` braced-init-lists are compiler magic: for a parameter of type
// `std::initializer_list<T>` the compiler materializes a temporary backing
// array plus a small (pointer, length) view object, no matter which library
// namespace the code otherwise draws from. There is no way for a user-defined
// class named anything other than `std::initializer_list` to receive that
// treatment, so CASTLE cannot invent its own brace-init hook.
//
// What CASTLE *can* do is:
//   - Pull in the compiler-support header when it exists. <initializer_list>
//     is header-only, allocates nothing and throws nothing, so it does not
//     compromise the "no heap / no exceptions" contract.
//   - Fall back to a hand-written, ABI-matching definition for bare-metal
//     toolchains that ship a freestanding compiler without that header.
//   - Expose the result as `castle::container::initializer_list<T>` together
//     with the iterator/algorithm glue the rest of CASTLE expects, so client
//     code never has to spell `std::` directly.
// ============================================================================

#if CASTLE_USING_STD_INITIALIZER_LIST
#include <initializer_list>
#else

namespace std
{

// Freestanding replacement for toolchains with no <initializer_list> header.
// This MUST stay named std::initializer_list: the compiler only performs
// brace-init-list magic (building a temporary array and constructing this
// object) for that exact qualified name, never for a castle-namespaced type,
// no matter how identical the layout is. The member layout (pointer first,
// element count second) mirrors what the Itanium C++ ABI requires the
// compiler to emit for brace-init-lists on GCC/Clang/ARM targets; only the
// compiler itself ever calls the private constructor below.
template <typename T>
class initializer_list
{
public:
    using value_type = T;
    using reference = CASTLE_CONST T&;
    using const_reference = CASTLE_CONST T&;
    using size_type = castle::size_type;
    using iterator = CASTLE_CONST T*;
    using const_iterator = CASTLE_CONST T*;

    CASTLE_CONSTEXPR initializer_list() CASTLE_NOEXCEPT : first_(nullptr), size_(0U) {}

    CASTLE_CONSTEXPR size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return size_; }
    CASTLE_CONSTEXPR CASTLE_CONST T* begin() CASTLE_CONST CASTLE_NOEXCEPT { return first_; }
    CASTLE_CONSTEXPR CASTLE_CONST T* end() CASTLE_CONST CASTLE_NOEXCEPT { return first_ + size_; }

private:
    CASTLE_CONSTEXPR initializer_list(CASTLE_CONST T* first, size_type size) CASTLE_NOEXCEPT
        : first_(first), size_(size)
    {
    }

    CASTLE_CONST T* first_;
    size_type size_;
};

} // namespace std

#endif // CASTLE_USING_STD_INITIALIZER_LIST

namespace castle
{
namespace container
{

// The type every CASTLE API spells when it wants to accept a braced-init-list.
// Always std::initializer_list<T> under the hood - real or the freestanding
// fallback above - so this alias needs no #if of its own.
template <typename T>
using initializer_list = CASTLE_STD::initializer_list<T>;

// A random-access, castle::iterator_traits-compatible reverse view. Backed by
// castle::reverse_iterator<const T*>, exactly like array/vector's own
// const_reverse_iterator, so generic code can treat every CASTLE container
// (and a brace-init-list) the same way.
template <typename T>
using const_reverse_initializer_iterator = castle::reverse_iterator<CASTLE_CONST T*>;

template <typename T>
CASTLE_CONSTEXPR CASTLE_CONST T* begin(initializer_list<T> list) CASTLE_NOEXCEPT
{
    return list.begin();
}

template <typename T>
CASTLE_CONSTEXPR CASTLE_CONST T* end(initializer_list<T> list) CASTLE_NOEXCEPT
{
    return list.end();
}

template <typename T>
CASTLE_CONSTEXPR castle::size_type size(initializer_list<T> list) CASTLE_NOEXCEPT
{
    return list.size();
}

template <typename T>
CASTLE_CONSTEXPR bool empty(initializer_list<T> list) CASTLE_NOEXCEPT
{
    return list.size() == 0U;
}

template <typename T>
CASTLE_CONSTEXPR CASTLE_CONST T& front(initializer_list<T> list) CASTLE_NOEXCEPT
{
    return *list.begin();
}

template <typename T>
CASTLE_CONSTEXPR CASTLE_CONST T& back(initializer_list<T> list) CASTLE_NOEXCEPT
{
    return *(list.end() - 1);
}

template <typename T>
CASTLE_CONSTEXPR const_reverse_initializer_iterator<T> rbegin(initializer_list<T> list) CASTLE_NOEXCEPT
{
    return const_reverse_initializer_iterator<T>(list.end());
}

template <typename T>
CASTLE_CONSTEXPR const_reverse_initializer_iterator<T> rend(initializer_list<T> list) CASTLE_NOEXCEPT
{
    return const_reverse_initializer_iterator<T>(list.begin());
}

} // namespace container
} // namespace castle

#endif // CASTLE_CONTAINER_INITIALIZER_LIST_HPP