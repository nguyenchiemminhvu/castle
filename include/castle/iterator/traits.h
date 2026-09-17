#ifndef CASTLE_ITERATOR_TRAITS_H
#define CASTLE_ITERATOR_TRAITS_H

#include "castle/core/compiler.h"
#include "castle/core/types.h"
#include "castle/iterator/tags.h"

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

#endif // CASTLE_ITERATOR_TRAITS_H
