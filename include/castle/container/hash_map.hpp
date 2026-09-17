// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file hash_map.hpp
 * @brief Fixed-capacity hash map alias backed by Castle's deterministic hash table.
 *
 * Use `hash_map<Key, T, N>` when key/value lookup should be hash-based, the
 * maximum number of live entries is known at compile time, and the design must
 * avoid heap allocation. Storage is embedded, capacity is fixed by `N`, and
 * insertion, lookup, and erasure use linear probing with O(N) worst-case cost.
 *
 * @code
 * castle::container::hash_map<int, int, 4U> values;
 * values.insert(1, 10);
 * const int* mapped = values.get(1);
 * @endcode
 */
#ifndef CASTLE_CONTAINER_HASH_MAP_HPP
#define CASTLE_CONTAINER_HASH_MAP_HPP

#include "castle/container/hash_table.hpp"

namespace castle
{
namespace container
{

/**
 * @brief Fixed-capacity hash map alias.
 * @tparam Key Key type.
 * @tparam T Mapped value type.
 * @tparam N Maximum number of live entries stored without dynamic allocation.
 * @tparam Hash Hash functor used to map keys to buckets.
 * @tparam KeyEqual Equality functor used to compare keys during probing.
 * @note This alias exposes the full `hash_table<Key, T, N, Hash, KeyEqual>`
 * API, including `insert()`, `try_emplace()`, `insert_or_assign()`, `find()`,
 * `get()`, `erase()`, and iteration.
 * @note Capacity queries are O(1). Insert, find, contains, get, and erase are
 * O(N) worst-case because probing may inspect every slot.
 * @note Erasing an entry invalidates iterators and references to that entry
 * only. Other entries do not move in memory; `clear()` invalidates everything.
 * @warning The container never reallocates, so capacity exhaustion is reported
 * with `castle::status::full` instead of growing the table.
 */
template <typename Key,
          typename T,
          size_type N,
          typename Hash = castle::hash<Key>,
          typename KeyEqual = castle::equal_to<Key>>
using hash_map = hash_table<Key, T, N, Hash, KeyEqual>;

}
}

#endif
