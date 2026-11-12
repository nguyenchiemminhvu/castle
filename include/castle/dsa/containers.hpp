// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file containers.hpp
 * @brief Umbrella include for Castle container headers.
 *
 * Include this header when a translation unit needs several Castle containers
 * and a single include is more convenient than spelling each dependency
 * individually. The header introduces no storage or runtime behavior of its
 * own; it simply exposes the fixed-capacity, allocation-free container family.
 *
 * @code
 * #include "castle/dsa/containers.hpp"
 *
 * castle::dsa::array<int, 2U> values{1, 2};
 * castle::dsa::string_view name("OK");
 * @endcode
 */
#ifndef CASTLE_DSA_CONTAINERS_HPP
#define CASTLE_DSA_CONTAINERS_HPP

#include "castle/dsa/array.hpp"
#include "castle/dsa/grid.hpp"
#include "castle/dsa/vector.hpp"
#include "castle/dsa/ring_buffer.hpp"
#include "castle/dsa/stack.hpp"
#include "castle/dsa/string.hpp"
#include "castle/dsa/string_view.hpp"
#include "castle/dsa/array_view.hpp"
#include "castle/dsa/heap.hpp"
#include "castle/dsa/avl_tree.hpp"
#include "castle/dsa/map.hpp"
#include "castle/dsa/set.hpp"
#include "castle/dsa/hash_table.hpp"
#include "castle/dsa/hash_map.hpp"
#include "castle/dsa/hash_set.hpp"
#include "castle/dsa/forward_list.hpp"
#include "castle/dsa/list.hpp"
#include "castle/dsa/disjoint_set.hpp"

#endif
