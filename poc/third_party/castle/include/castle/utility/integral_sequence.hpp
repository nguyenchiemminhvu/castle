// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file integral_sequence.hpp
 * @brief Compile-time index-sequence helpers for Castle metaprogramming.
 *
 * Use this header when tuple-like code or other compile-time utilities need a
 * sequence of indices without relying on `<utility>`. Every facility is purely
 * type-based, evaluated at compile time, and introduces no runtime storage or
 * allocation.
 *
 * @code
 * using seq = castle::make_index_sequence_t<4U>;
 * static_assert(seq::size() == 4U, "sequence size");
 * @endcode
 */
#ifndef CASTLE_CORE_INTEGRAL_SEQUENCE_HPP
#define CASTLE_CORE_INTEGRAL_SEQUENCE_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/core/traits.hpp"

namespace castle
{
namespace sequence
{

/**
 * @brief Stores a compile-time sequence of `size_type` indices.
 * @tparam Indices Sequence elements.
 */
template <size_type... Indices>
struct index_sequence
{
    using type = index_sequence;
    using value_type = size_type;

    /**
     * @brief Returns the number of indices in the sequence.
     * @return Sequence length.
     */
    static CASTLE_CONSTEXPR size_type size() CASTLE_NOEXCEPT
    {
        return sizeof...(Indices);
    }
};

/**
 * @brief Concatenates two index sequences and renumbers the second half.
 * @tparam Left Left input sequence.
 * @tparam Right Right input sequence.
 */
template <typename Left, typename Right>
struct merge_and_renumber;

template <size_type... L, size_type... R>
struct merge_and_renumber<
    index_sequence<L...>,
    index_sequence<R...>>
{
    using type = index_sequence<
        L...,
        (sizeof...(L) + R)...
    >;
};

/**
 * @brief Builds `index_sequence<0, 1, ..., N - 1>`.
 * @tparam N Number of elements in the sequence.
 */
template <size_type N>
struct make_index_sequence
    : merge_and_renumber<
          typename make_index_sequence<N / 2>::type,
          typename make_index_sequence<N - N / 2>::type>
{
};

template <>
struct make_index_sequence<0>
{
    using type = index_sequence<>;
};

template <>
struct make_index_sequence<1>
{
    using type = index_sequence<0>;
};

/**
 * @brief Convenience alias for the `make_index_sequence<N>::type` result.
 * @tparam N Number of elements in the sequence.
 */
template <size_type N>
using make_index_sequence_t =
    typename make_index_sequence<N>::type;

/**
 * @brief Builds an index sequence with one index per type in `Ts...`.
 * @tparam Ts Type pack used to determine the length.
 */
template <typename... Ts>
using index_sequence_for =
    make_index_sequence_t<sizeof...(Ts)>;

/**
 * @brief Returns the size of an index sequence type.
 * @tparam Sequence Sequence to inspect.
 */
template <typename Sequence>
struct sequence_size;

template <size_type... Indices>
struct sequence_size<index_sequence<Indices...>>
    : integral_constant<size_type, sizeof...(Indices)>
{
};

/**
 * @brief Alias for `sequence_size<Sequence>`.
 * @tparam Sequence Sequence to inspect.
 */
template <typename Sequence>
using sequence_size_t = sequence_size<Sequence>;

/**
 * @brief Detects whether a type is a Castle index sequence.
 * @tparam Sequence Candidate type.
 */
template <typename Sequence>
struct is_index_sequence : false_type
{
};

template <size_type... Indices>
struct is_index_sequence<index_sequence<Indices...>> : true_type
{
};

/**
 * @brief Boolean constant indicating whether `Sequence` is an index sequence.
 * @tparam Sequence Candidate type.
 */
template <typename Sequence>
CASTLE_CONSTEXPR bool is_index_sequence_v = is_index_sequence<Sequence>::value;

} // namespace sequence

/**
 * @brief Top-level alias for `sequence::index_sequence`.
 * @tparam Indices Sequence elements.
 */
template <size_type... Indices>
using index_sequence = sequence::index_sequence<Indices...>;

/**
 * @brief Top-level alias for `sequence::make_index_sequence`.
 * @tparam N Number of elements in the sequence.
 */
template <size_type N>
using make_index_sequence = sequence::make_index_sequence<N>;
/**
 * @brief Convenience alias for the generated index-sequence type.
 * @tparam N Number of elements in the sequence.
 */
template <size_type N>
using make_index_sequence_t = typename make_index_sequence<N>::type;

/**
 * @brief Boolean constant indicating whether `Sequence` is an index sequence.
 * @tparam Sequence Candidate type.
 */
template <typename Sequence>
CASTLE_CONSTEXPR bool is_index_sequence = sequence::is_index_sequence_v<Sequence>;

} // namespace castle

#endif // CASTLE_CORE_INTEGRAL_SEQUENCE_HPP
