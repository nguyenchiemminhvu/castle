// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file tuple.hpp
 * @brief Fixed-size heterogeneous tuple utilities for Castle.
 *
 * Use this header when generic code needs compile-time indexed storage,
 * reference tuples, or unique-type lookup without the standard library.
 * Storage is fully static, no allocation occurs, and tuple operations stay
 * deterministic and exception-free.
 *
 * @code
 * auto packet = castle::make_tuple(10U, 20U);
 * auto second = castle::get<1>(packet);
 * @endcode
 */
#ifndef CASTLE_UTILITY_TUPLE_HPP
#define CASTLE_UTILITY_TUPLE_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/types.hpp"
#include "castle/core/traits.hpp"
#include "castle/utility/integral_sequence.hpp"
#include "castle/utility/move.hpp"
#include "castle/utility/forward.hpp"

namespace castle
{

template <typename... Ts>
class tuple;

/**
 * @brief Maps a tuple index to the corresponding element type.
 * @tparam Index Zero-based tuple element index.
 * @tparam Tuple Tuple type to inspect.
 */
template <size_type Index, typename Tuple>
struct tuple_element;

template <size_type Index, typename Head, typename... Tail>
struct tuple_element<Index, castle::tuple<Head, Tail...>>
    : tuple_element<Index - 1U, castle::tuple<Tail...>>
{
};

template <typename Head, typename... Tail>
struct tuple_element<0U, castle::tuple<Head, Tail...>>
{
    using type = Head;
};

/**
 * @brief Convenience alias for `tuple_element<Index, Tuple>::type`.
 * @tparam Index Zero-based tuple element index.
 * @tparam Tuple Tuple type to inspect.
 */
template <size_type Index, typename Tuple>
using tuple_element_t = typename tuple_element<Index, Tuple>::type;

/**
 * @brief Returns the compile-time number of elements in a tuple type.
 * @tparam Tuple Tuple type to inspect.
 */
template <typename Tuple>
struct tuple_size;

template <typename... Ts>
struct tuple_size<castle::tuple<Ts...>>
    : integral_constant<size_type, sizeof...(Ts)>
{
};

namespace detail
{

template <size_type Index, typename T>
struct tuple_leaf
{
    T value;

    CASTLE_CONSTEXPR tuple_leaf()
        : value()
    {
    }

    template <typename U>
    CASTLE_CONSTEXPR explicit tuple_leaf(U&& other)
        : value(CASTLE_FORWARD<U>(other))
    {
    }
};

template <typename Sequence, typename... Ts>
struct tuple_impl;

template <size_type... Indices, typename... Ts>
struct tuple_impl<sequence::index_sequence<Indices...>,
    Ts...>
    : tuple_leaf<Indices, Ts>...
{
    CASTLE_CONSTEXPR tuple_impl()
        : tuple_leaf<Indices, Ts>()...
    {
    }

    template <typename... Us>
    CASTLE_CONSTEXPR explicit tuple_impl(Us&&... values)
        : tuple_leaf<Indices, Ts>(
              CASTLE_FORWARD<Us>(values))...
    {
    }
};

} 

/**
 * @brief Stores a fixed set of heterogeneous values.
 * @tparam Ts Element types stored by the tuple.
 */
template <typename... Ts>
class tuple
    : public detail::tuple_impl<
          sequence::index_sequence_for<Ts...>,
          Ts...>
{
private:

    using base_type =
        detail::tuple_impl<
            sequence::index_sequence_for<Ts...>,
            Ts...>;

public:

    using size_type = castle::size_type;

    
    /** @brief Returns the number of elements in the tuple. */
    static CASTLE_CONSTEXPR size_type size() CASTLE_NOEXCEPT
    {
        return sizeof...(Ts);
    }

    /** @brief Default-constructs the tuple when every element can be default-constructed. */
    CASTLE_CONSTEXPR tuple() CASTLE_DEFAULT;

    /**
     * @brief Constructs every tuple element from the supplied arguments.
     * @tparam Us Source types forwarded to the tuple elements.
     * @param values Values forwarded into the stored elements.
     */
    template <
        typename... Us,
        typename meta::enable_if<
            sizeof...(Us) == sizeof...(Ts),
            int>::type = 0>
    CASTLE_CONSTEXPR explicit tuple(Us&&... values)
        : base_type(CASTLE_FORWARD<Us>(values)...)
    {
    }

    tuple(CASTLE_CONST tuple&) CASTLE_DEFAULT;
    tuple(tuple&&) CASTLE_DEFAULT;

    tuple& operator=(CASTLE_CONST tuple&) CASTLE_DEFAULT;
    tuple& operator=(tuple&&) CASTLE_DEFAULT;

    ~tuple() CASTLE_DEFAULT;
};

/**
 * @brief Returns a tuple element by compile-time index.
 * @tparam Index Zero-based element index.
 * @tparam Ts Tuple element types.
 * @param value Tuple to inspect.
 * @return Reference to the selected element.
 */
template <size_type Index, typename... Ts>
CASTLE_CONSTEXPR
castle::tuple_element_t<Index, tuple<Ts...>>&
get(tuple<Ts...>& value) CASTLE_NOEXCEPT
{
    using value_type = castle::tuple_element_t<Index, tuple<Ts...>>;
    using leaf_type = detail::tuple_leaf<Index, value_type>;

    return static_cast<leaf_type&>(value).value;
}

/** @brief Returns a const tuple element by compile-time index. */
template <size_type Index, typename... Ts>
CASTLE_CONSTEXPR
CASTLE_CONST castle::tuple_element_t<Index, tuple<Ts...>>&
get(CASTLE_CONST tuple<Ts...>& value) CASTLE_NOEXCEPT
{
    using value_type = castle::tuple_element_t<Index, tuple<Ts...>>;
    using leaf_type = detail::tuple_leaf<Index, value_type>;

    return static_cast<CASTLE_CONST leaf_type&>(value).value;
}

/** @brief Returns a moved tuple element by compile-time index. */
template <size_type Index, typename... Ts>
CASTLE_CONSTEXPR
castle::tuple_element_t<Index, tuple<Ts...>>&&
get(tuple<Ts...>&& value) CASTLE_NOEXCEPT
{
    return CASTLE_MOVE(get<Index>(value));
}

/** @brief Returns a moved const tuple element by compile-time index. */
template <size_type Index, typename... Ts>
CASTLE_CONSTEXPR
CASTLE_CONST castle::tuple_element_t<Index, tuple<Ts...>>&&
get(CASTLE_CONST tuple<Ts...>&& value) CASTLE_NOEXCEPT
{
    return CASTLE_MOVE(get<Index>(value));
}

namespace meta
{

/**
 * @brief Counts how many times `T` appears in a tuple type list.
 * @tparam T Type to count.
 * @tparam Ts Candidate tuple element types.
 */
template <typename T, typename... Ts>
struct tuple_type_count;

template <typename T>
struct tuple_type_count<T>
    : meta::integral_constant<size_type, 0U>
{
};

template <typename T, typename Head, typename... Tail>
struct tuple_type_count<T, Head, Tail...>
    : meta::integral_constant<
          size_type,
          (meta::is_same<T, Head>::value ? 1U : 0U)
              + tuple_type_count<T, Tail...>::value>
{
};

/**
 * @brief Finds the first index of `T` in a tuple type list.
 * @tparam T Type to locate.
 * @tparam Ts Candidate tuple element types.
 */
template <typename T, typename... Ts>
struct tuple_type_index;

template <typename T, typename Head, typename... Tail>
struct tuple_type_index<T, Head, Tail...>
{
    static CASTLE_CONSTEXPR size_type value =
        meta::is_same<T, Head>::value
            ? 0U
            : 1U + tuple_type_index<T, Tail...>::value;
};

template <typename T>
struct tuple_type_index<T>
{
    static CASTLE_CONSTEXPR size_type value = 0U;
};

} 

/**
 * @brief Returns a tuple element by unique element type.
 * @tparam T Element type to retrieve.
 * @tparam Ts Tuple element types.
 * @param value Tuple to inspect.
 * @return Reference to the selected element.
 * @warning `T` must appear exactly once in the tuple.
 */
template <typename T, typename... Ts>
CASTLE_CONSTEXPR T&
get(tuple<Ts...>& value) CASTLE_NOEXCEPT
{
    static_assert(
        meta::tuple_type_count<T, Ts...>::value == 1U,
        "castle::get<T> requires T to occur exactly once in tuple"
    );

    return castle::get<meta::tuple_type_index<T, Ts...>::value>(value);
}

/** @brief Returns a const tuple element by unique element type. */
template <typename T, typename... Ts>
CASTLE_CONSTEXPR CASTLE_CONST T&
get(CASTLE_CONST tuple<Ts...>& value) CASTLE_NOEXCEPT
{
    static_assert(
        meta::tuple_type_count<T, Ts...>::value == 1U,
        "castle::get<T> requires T to occur exactly once in tuple"
    );

    return castle::get<meta::tuple_type_index<T, Ts...>::value>(value);
}

/** @brief Returns a moved tuple element by unique element type. */
template <typename T, typename... Ts>
CASTLE_CONSTEXPR T&&
get(tuple<Ts...>&& value) CASTLE_NOEXCEPT
{
    static_assert(
        meta::tuple_type_count<T, Ts...>::value == 1U,
        "castle::get<T> requires T to occur exactly once in tuple"
    );

    return castle::get<
        meta::tuple_type_index<T, Ts...>::value>(
            CASTLE_MOVE(value));
}

/** @brief Returns a moved const tuple element by unique element type. */
template <typename T, typename... Ts>
CASTLE_CONSTEXPR CASTLE_CONST T&&
get(CASTLE_CONST tuple<Ts...>&& value) CASTLE_NOEXCEPT
{
    static_assert(
        meta::tuple_type_count<T, Ts...>::value == 1U,
        "castle::get<T> requires T to occur exactly once in tuple"
    );

    return castle::get<
        meta::tuple_type_index<T, Ts...>::value>(
            CASTLE_MOVE(value));
}

/**
 * @brief Convenience variable template exposing `tuple_size<Tuple>::value`.
 * @tparam Tuple Tuple type to inspect.
 */
template <typename Tuple>
inline CASTLE_CONSTEXPR size_type tuple_size_v = castle::tuple_size<Tuple>::value;

template <size_type Index, typename Tuple>
using tuple_element_t = typename castle::tuple_element<Index, Tuple>::type;

/**
 * @brief Creates a tuple with decayed element types.
 * @tparam Ts Source value types.
 * @param values Values forwarded into the tuple.
 * @return A tuple containing the decayed values.
 */
template <typename... Ts>
CASTLE_CONSTEXPR
tuple<meta::decay_t<Ts>...>
make_tuple(Ts&&... values)
{
    return tuple<meta::decay_t<Ts>...>(CASTLE_FORWARD<Ts>(values)...);
}

/**
 * @brief Creates a tuple of lvalue references.
 * @tparam Ts Referenced value types.
 * @param values Values to reference.
 * @return Tuple holding lvalue references to `values`.
 */
template <typename... Ts>
CASTLE_CONSTEXPR
tuple<Ts&...>
tie(Ts&... values) CASTLE_NOEXCEPT
{
    return tuple<Ts&...>(values...);
}

/**
 * @brief Assignment sink used by tuple-like code that wants to discard values.
 */
struct ignore_t
{
    template <typename T>
    CASTLE_CONSTEXPR
    /** @brief Discards an assigned value and returns `*this`. */
    ignore_t& operator=(T&&) CASTLE_NOEXCEPT
    {
        return *this;
    }
};

/** @brief Global ignore sink object. */
inline ignore_t ignore{};

/**
 * @brief Creates a tuple of forwarding references.
 * @tparam Ts Source value types.
 * @param values Values whose reference categories should be preserved.
 * @return Tuple of forwarding references.
 */
template <typename... Ts>
CASTLE_CONSTEXPR
tuple<Ts&&...>
forward_as_tuple(Ts&&... values) CASTLE_NOEXCEPT
{
    return tuple<Ts&&...>(CASTLE_FORWARD<Ts>(values)...);
}

} 

#endif 