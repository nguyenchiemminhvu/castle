// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @brief Iterator category tags used by Castle iterator traits and algorithms.
 *
 * Use these empty marker types to classify custom iterators without depending on
 * `<iterator>`. Castle algorithms dispatch on this inheritance chain to select the
 * strongest operation set supported by an iterator category while remaining allocation-free
 * and deterministic.
 *
 * @code
 * void scan(castle::input_iterator_tag);
 * void scan(castle::random_access_iterator_tag);
 *
 * scan(castle::random_access_iterator_tag{});
 * @endcode
 */
#ifndef CASTLE_ITERATOR_TAGS_HPP
#define CASTLE_ITERATOR_TAGS_HPP

#include "castle/core/compiler.hpp"

namespace castle
{

/**
 * @brief Category tag for single-pass readable iterators.
 * @note Serves as the base class for stronger Castle iterator categories.
 */
struct input_iterator_tag {};

/**
 * @brief Category tag for write-only output iterators.
 * @note This tag is independent from the readable iterator hierarchy.
 */
struct output_iterator_tag {};

/**
 * @brief Category tag for multipass forward iterators.
 * @note Inherits from `input_iterator_tag`.
 */
struct forward_iterator_tag : input_iterator_tag {};

/**
 * @brief Category tag for iterators that support forward and backward movement.
 * @note Inherits from `forward_iterator_tag`.
 */
struct bidirectional_iterator_tag : forward_iterator_tag {};

/**
 * @brief Category tag for iterators that additionally support random access arithmetic.
 * @note Inherits from `bidirectional_iterator_tag`.
 */
struct random_access_iterator_tag : bidirectional_iterator_tag {};

}

#endif
