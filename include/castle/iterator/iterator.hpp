// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @brief Umbrella header for Castle iterator primitives and adaptors.
 *
 * Include this header when a translation unit needs several Castle iterator facilities at
 * once and the additional include cost is acceptable. It re-exports iterator algorithms,
 * reverse iteration, circular traversal, fixed-range traversal, and iterator trait support
 * without introducing allocation, exceptions, RTTI, or STL iterator dependencies.
 *
 * @code
 * #include "castle/iterator/iterator.hpp"
 *
 * uint8_t values[3U] = {1U, 2U, 3U};
 * auto second = castle::next(values, 1);
 * castle::reverse_iterator<uint8_t*> reverse_end(values + 3U);
 * (void)second;
 * (void)reverse_end;
 * @endcode
 */
#ifndef CASTLE_ITERATOR_ITERATOR_HPP
#define CASTLE_ITERATOR_ITERATOR_HPP

#include "castle/iterator/operations.hpp"
#include "castle/iterator/reverse_iterator.hpp"
#include "castle/iterator/circular_iterator.hpp"
#include "castle/iterator/fixed_iterator.hpp"
#include "castle/iterator/traits.hpp"

#endif
