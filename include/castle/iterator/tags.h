#ifndef CASTLE_ITERATOR_TAGS_H
#define CASTLE_ITERATOR_TAGS_H

#include "castle/core/compiler.h"

namespace castle
{

struct input_iterator_tag {};
struct output_iterator_tag {};
struct forward_iterator_tag : input_iterator_tag {};
struct bidirectional_iterator_tag : forward_iterator_tag {};
struct random_access_iterator_tag : bidirectional_iterator_tag {};

} // namespace castle

#endif // CASTLE_ITERATOR_TAGS_H
