#ifndef CASTLE_ITERATOR_TAGS_HPP
#define CASTLE_ITERATOR_TAGS_HPP

#include "castle/core/compiler.hpp"

namespace castle
{

struct input_iterator_tag {};
struct output_iterator_tag {};
struct forward_iterator_tag : input_iterator_tag {};
struct bidirectional_iterator_tag : forward_iterator_tag {};
struct random_access_iterator_tag : bidirectional_iterator_tag {};

} // namespace castle

#endif // CASTLE_ITERATOR_TAGS_HPP
