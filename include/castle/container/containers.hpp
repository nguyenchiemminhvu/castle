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
 * #include "castle/container/containers.hpp"
 *
 * castle::container::array<int, 2U> values{1, 2};
 * castle::container::string_view name("OK");
 * @endcode
 */
#ifndef CASTLE_CONTAINER_CONTAINERS_HPP
#define CASTLE_CONTAINER_CONTAINERS_HPP

#include "castle/container/array.hpp"
#include "castle/container/vector.hpp"
#include "castle/container/ring_buffer.hpp"
#include "castle/container/stack.hpp"
#include "castle/container/string.hpp"
#include "castle/container/string_view.hpp"
#include "castle/container/array_view.hpp"
#include "castle/container/heap.hpp"
#include "castle/container/avl_tree.hpp"
#include "castle/container/map.hpp"
#include "castle/container/set.hpp"
#include "castle/container/hash_table.hpp"
#include "castle/container/hash_map.hpp"
#include "castle/container/hash_set.hpp"
#include "castle/container/forward_list.hpp"

#endif
