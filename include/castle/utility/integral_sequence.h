#ifndef CASTLE_CORE_INTEGRAL_SEQUENCE_H
#define CASTLE_CORE_INTEGRAL_SEQUENCE_H

#include "castle/core/compiler.h"
#include "castle/core/types.h"
#include "castle/core/traits.h"

namespace castle
{
namespace sequence
{

// ============================================================================
// index_sequence
// ============================================================================

template <size_type... Indices>
struct index_sequence
{
    using type = index_sequence;
    using value_type = size_type;

    static CASTLE_CONSTEXPR size_type size() CASTLE_NOEXCEPT
    {
        return sizeof...(Indices);
    }
};

// ============================================================================
// merge_and_renumber
//
// [0, 1, 2] + [0, 1]
// becomes
// [0, 1, 2, 3, 4]
// ============================================================================

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

// ============================================================================
// make_index_sequence
// ============================================================================

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

// Convenience alias.

template <size_type N>
using make_index_sequence_t =
    typename make_index_sequence<N>::type;

// ============================================================================
// index_sequence_for
// ============================================================================

template <typename... Ts>
using index_sequence_for =
    make_index_sequence_t<sizeof...(Ts)>;

// ============================================================================
// Additional sequence helpers
// ============================================================================

template <typename Sequence>
struct sequence_size;

template <size_type... Indices>
struct sequence_size<index_sequence<Indices...>>
    : integral_constant<size_type, sizeof...(Indices)>
{
};

template <typename Sequence>
using sequence_size_t = sequence_size<Sequence>;

// ============================================================================
// index sequence traits
// ============================================================================

template <typename Sequence>
struct is_index_sequence : false_type
{
};

template <size_type... Indices>
struct is_index_sequence<index_sequence<Indices...>> : true_type
{
};

template <typename Sequence>
CASTLE_CONSTEXPR bool is_index_sequence_v = is_index_sequence<Sequence>::value;

} // namespace sequence

// Alias for index_sequence
template <size_type... Indices>
using index_sequence = sequence::index_sequence<Indices...>;

// Alias for make_index_sequence
template <size_type N>
using make_index_sequence = sequence::make_index_sequence<N>;
// Convenience alias for the type
template <size_type N>
using make_index_sequence_t = typename make_index_sequence<N>::type;

// Alias for is_index_sequence
template <typename Sequence>
CASTLE_CONSTEXPR bool is_index_sequence = sequence::is_index_sequence_v<Sequence>;

} // namespace castle

#endif // CASTLE_CORE_INTEGRAL_SEQUENCE_H
