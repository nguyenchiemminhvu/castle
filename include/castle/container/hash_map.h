#ifndef CASTLE_CONTAINER_HASH_MAP_H
#define CASTLE_CONTAINER_HASH_MAP_H

#include "castle/container/hash_table.h"

namespace castle
{
namespace container
{

template <typename Key,
          typename T,
          size_type N,
          typename Hash = castle::hash<Key>,
          typename KeyEqual = castle::equal_to<Key>>
using hash_map = hash_table<Key, T, N, Hash, KeyEqual>;

} // namespace container
} // namespace castle

#endif // CASTLE_CONTAINER_HASH_MAP_H
