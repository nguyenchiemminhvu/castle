#ifndef CASTLE_ITERATOR_TRAITS_HPP
#define CASTLE_ITERATOR_TRAITS_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/iterator/tags.hpp"

namespace castle
{

template <typename Iterator>
struct iterator_traits
{
    using difference_type = typename Iterator::difference_type;
    using value_type = typename Iterator::value_type;
    using pointer = typename Iterator::pointer;
    using reference = typename Iterator::reference;
    using iterator_category = typename Iterator::iterator_category;
};

template <typename T>
struct iterator_traits<T*>
{
    using difference_type = castle::difference_type;
    using value_type = T;
    using pointer = T*;
    using reference = T&;
    using iterator_category = random_access_iterator_tag;
};

template <typename T>
struct iterator_traits<CASTLE_CONST T*>
{
    using difference_type = castle::difference_type;
    using value_type = T;
    using pointer = CASTLE_CONST T*;
    using reference = CASTLE_CONST T&;
    using iterator_category = random_access_iterator_tag;
};

} // namespace castle

#endif // CASTLE_ITERATOR_TRAITS_HPP
