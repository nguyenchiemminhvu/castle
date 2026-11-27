// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @brief Lightweight iterator trait extraction for Castle iterators and raw pointers.
 *
 * Use this header in generic embedded code that must discover iterator-associated types
 * without depending on `<iterator>`. The primary template reads nested aliases from custom
 * iterator types, while pointer specializations model raw pointers as random-access
 * iterators. The trait reports types only; it does not manage storage or change iterator
 * invalidation rules.
 *
 * @code
 * using pointer_traits = castle::iterator_traits<uint8_t*>;
 * pointer_traits::iterator_category category{};
 * (void)category;
 * @endcode
 */
#ifndef CASTLE_ITERATOR_TRAITS_HPP
#define CASTLE_ITERATOR_TRAITS_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/iterator/tags.hpp"

namespace castle
{

/**
 * @brief Extracts the standard Castle iterator-associated types from a custom iterator.
 * @tparam Iterator Iterator type that exposes nested `difference_type`, `value_type`,
 *                  `pointer`, `reference`, and `iterator_category` aliases.
 * @note This trait only aliases types; iterator validity and invalidation remain the
 *       responsibility of the underlying iterator implementation.
 */
template <typename Iterator>
struct iterator_traits
{
    using difference_type = typename Iterator::difference_type;
    using value_type = typename Iterator::value_type;
    using pointer = typename Iterator::pointer;
    using reference = typename Iterator::reference;
    using iterator_category = typename Iterator::iterator_category;
};

/**
 * @brief Specialization that models mutable raw pointers as random-access iterators.
 * @tparam T Pointed-to element type.
 * @note `value_type` is `T`, `pointer` is `T*`, `reference` is `T&`, and the category is
 *       `random_access_iterator_tag`.
 */
template <typename T>
struct iterator_traits<T*>
{
    using difference_type = castle::difference_type;
    using value_type = T;
    using pointer = T*;
    using reference = T&;
    using iterator_category = random_access_iterator_tag;
};

/**
 * @brief Specialization that models pointers to constant elements as random-access iterators.
 * @tparam T Pointed-to element type.
 * @note `value_type` remains `T`, while `pointer` and `reference` preserve element constness.
 */
template <typename T>
struct iterator_traits<CASTLE_CONST T*>
{
    using difference_type = castle::difference_type;
    using value_type = T;
    using pointer = CASTLE_CONST T*;
    using reference = CASTLE_CONST T&;
    using iterator_category = random_access_iterator_tag;
};

}

#endif
