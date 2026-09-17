#pragma once

#include <cstddef>
#include <iterator>
#include <type_traits>

// ─────────────────────────────────────────────────────────────────────────────
// iterator_traits — associated-type access + iterator category tags.
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Castle iterators are logical cursors rather than raw pointers, so generic
 * algorithms cannot rely on pointer arithmetic. iterator_traits provides the
 * uniform type access those algorithms need, and the category tags let them
 * dispatch on traversal capability.
 *
 * The category tags are aliased to the std tags (not redefined) so a Castle
 * iterator is also a valid std iterator: std::iterator_traits sees a real std
 * category and STL algorithms (std::find, std::copy, std::accumulate,
 * std::distance, ...) work directly on Castle buffers. The std hierarchy is
 * identical, so Castle's own is_base_of<> dispatch is unaffected:
 *
 *     input          output
 *       |
 *     forward
 *       |
 *     bidirectional
 *       |
 *     random_access
 */

namespace castle
{
namespace iterators
{

// ── iterator category tags ──────────────────────────────────────────────────

// Aliased to the std tags (not redefined) so a Castle iterator is also a valid
// std iterator: std::iterator_traits sees a real std category and STL
// algorithms work directly on Castle buffers. The std hierarchy is identical,
// so Castle's own is_base_of<> dispatch is unaffected.
using input_iterator_tag         = std::input_iterator_tag;
using output_iterator_tag        = std::output_iterator_tag;
using forward_iterator_tag       = std::forward_iterator_tag;
using bidirectional_iterator_tag = std::bidirectional_iterator_tag;
using random_access_iterator_tag = std::random_access_iterator_tag;

// ── iterator_traits ─────────────────────────────────────────────────────────

// Primary template: empty when the type does not model an iterator. This lets
// SFINAE-based overloads reject non-iterators gracefully.
template <typename Iterator, typename = void>
struct iterator_traits
{
};

// A type is treated as an iterator when it publishes the five member typedefs.
template <typename Iterator>
struct iterator_traits<
    Iterator,
    std::void_t<
        typename Iterator::iterator_category,
        typename Iterator::value_type,
        typename Iterator::difference_type,
        typename Iterator::pointer,
        typename Iterator::reference>>
{
    using iterator_category = typename Iterator::iterator_category;
    using value_type        = typename Iterator::value_type;
    using difference_type   = typename Iterator::difference_type;
    using pointer           = typename Iterator::pointer;
    using reference         = typename Iterator::reference;
};

// Raw pointers are random-access iterators.
template <typename T>
struct iterator_traits<T*>
{
    using iterator_category = random_access_iterator_tag;
    using value_type        = T;
    using difference_type   = std::ptrdiff_t;
    using pointer           = T*;
    using reference         = T&;
};

template <typename T>
struct iterator_traits<const T*>
{
    using iterator_category = random_access_iterator_tag;
    using value_type        = T;
    using difference_type   = std::ptrdiff_t;
    using pointer           = const T*;
    using reference         = const T&;
};

} // namespace iterators
} // namespace castle
