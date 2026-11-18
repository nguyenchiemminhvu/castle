# trie

## Overview
Fixed-capacity prefix tree (trie) for keys made of symbols from a small alphabet: dictionaries, autocomplete, dial plans, command tables, and board word searches such as [LeetCode 212, Word Search II](https://leetcode.com/problems/word-search-ii/).

The trie is bounded the way tries are naturally shaped:

- **Width.** Each node has one child slot per alphabet symbol (26 for `lowercase_alphabet`, 10 for `digit_alphabet`, or any custom alphabet). Stepping to a child is one array lookup, O(1).
- **Depth.** `MaxDepth` is the longest key, which is the number of node layers below the root.
- **Keys.** `MaxKeys` is how many keys can be stored at once. By default it is every key the alphabet can form up to `MaxDepth` symbols, which gives a complete trie.

The node pool is sized from these three bounds at compile time, so any `MaxKeys` keys of length at most `MaxDepth` always fit. Insertion never fails for lack of nodes. Erasing a key frees the nodes no other key needs, and freed nodes are reused.

## Header
`#include "castle/dsa/trie.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [status](../core/status.md)
- [traits](../core/traits.md) (`meta::smallest_uint_t`, `meta::make_unsigned_t`)
- [types](../core/types.md)
- [array](array.md): node storage
- [string_view](string_view.md): `basic_string_view` key type

## Public API
### Alphabets
| API | Description |
| --- | --- |
| `contiguous_alphabet<Symbol, First, Size>` | `Size` consecutive symbol values starting at `First`. `index_of(s)` returns `s - First`, or a value `>= Size` for a symbol outside the alphabet. |
| `lowercase_alphabet` | `contiguous_alphabet<char, 'a', 26>`. |
| `digit_alphabet` | `contiguous_alphabet<char, '0', 10>`. |
| Custom alphabet | Any type with `symbol_type`, `static constexpr size_type size`, and `static size_type index_of(symbol_type)` that returns `0 .. size - 1`, or `>= size` when the symbol is rejected. Example: DNA bases `A C G T` with width 4. |

### `trie<Alphabet, MaxDepth, MaxKeys = complete, Value = uint32_t>`
| API | Description |
| --- | --- |
| `symbol_type`, `key_type` | Alphabet symbol type and `basic_string_view<symbol_type>`. Null-terminated strings convert implicitly. Use `key_type(pointer, length)` for other buffers. |
| `node_handle` | Node index: the smallest unsigned type that holds `static_node_capacity`. |
| `count_type` | Key counter: the smallest unsigned type that holds `MaxKeys`. |
| `static_width`, `static_max_depth`, `static_capacity` | `Alphabet::size`, `MaxDepth`, `MaxKeys`. |
| `static_node_capacity` | Pool size, root included: `1 + Σ min(width^d, MaxKeys)` for `d = 1 .. MaxDepth`. |
| `npos` | Handle returned for a missing prefix or child. |
| `status insert(key, value = Value())` | Stores a key. Returns `already_exists` (and keeps the old payload), `full` when `size() == MaxKeys`, `out_of_range` when `key.size() > MaxDepth`, or `invalid_argument` for a symbol outside the alphabet. A rejected insert changes nothing. O(L). |
| `status erase(key)` | Removes a key and frees every node that no remaining key passes through. Returns `not_found` when the key is not stored. O(L). |
| `bool contains(key)` | `true` when the exact key is stored. O(L). |
| `bool contains_prefix(prefix)` | `true` when some stored key starts with `prefix`. O(L). |
| `size_type prefix_count(prefix)` | Number of stored keys that start with `prefix`, the prefix itself included. O(L). |
| `node_handle find(prefix)` | Node for `prefix`, or `npos` when no stored key starts with it. O(L). |
| `node_handle root()` | Root handle, which is the empty prefix. |
| `node_handle advance(from, symbol)` | Child of `from` along `symbol`, or `npos`. Accepts `npos` as `from`, so calls can be chained. O(1). |
| `bool is_terminal(handle)` | `true` when a stored key ends at `handle`. O(1). |
| `size_type prefix_count(handle)` | Keys stored at or below `handle`. O(1). |
| `status try_value(handle, out)` | Copies the payload of the key ending at `handle`, or returns `not_found`. O(1). |
| `void clear()` | Removes all keys. O(width). Invalidates every handle. |
| `size()`, `capacity()`, `empty()`, `full()` | Key count, `MaxKeys`, and the two tests. |
| `max_depth()` | `MaxDepth`. |
| `node_count()`, `node_capacity()` | Nodes in use (root included) and `static_node_capacity`. |

L is the key or prefix length.

## Usage Example
```cpp
#include "castle/dsa/trie.hpp"

// Up to 16 words of at most 10 lowercase letters; payload = word index.
castle::dsa::trie<castle::dsa::lowercase_alphabet, 10U, 16U, uint16_t> words;
words.insert("oath", 0U);
words.insert("oat", 1U);
words.prefix_count("oa");          // 2
words.contains("oa");              // false

// Walk symbol by symbol, as a board search does.
auto node = words.advance(words.root(), 'o');
node = words.advance(node, 'a');
node = words.advance(node, 't');
uint16_t id = 0U;
if (words.try_value(node, id) == castle::status::ok)
{
    words.erase("oat");            // report once; nodes still used by "oath" stay
}
```

`samples/castle/dsa/trie.cpp` solves Word Search II on both LeetCode examples. The depth-first search follows the trie with `advance()` and erases each word as soon as it is found. Erasing frees the branches that no other word needs, so the search stops exploring them. The sample also shows a digit dial-plan with prefix counts.

## Constraints & Notes
- No allocation, exceptions, RTTI, or virtual dispatch. Insert, erase, and lookup are iterative and use O(1) stack.
- **Node layout.** Each node holds `width` child handles, a `count_type` key counter, a terminal flag, and one `Value`. The object size is roughly `static_node_capacity * (width * sizeof(node_handle) + sizeof(count_type) + 1 + sizeof(Value))`.
- **Why the pool bound holds.** Every node in use lies on the path of at least one stored key. Erase enforces this. At depth `d` there are at most `width^d` distinct prefixes, and no more nodes than stored keys. So layer `d` never needs more than `min(width^d, MaxKeys)` nodes.
- **Sizing examples.**

  | Instantiation | `static_node_capacity` | Note |
  | --- | --- | --- |
  | `trie<digit_alphabet, 3>` | 1 111 | complete trie of every 0–3 digit key |
  | `trie<lowercase_alphabet, 8, 16>` | 129 | 1 + 8 × 16: every layer is capped at 16 keys; about 4 KiB |
  | `trie<lowercase_alphabet, 10, 30000, uint16_t>` | 228 279 | LeetCode 212 limits; about 25.6 MB with 4-byte handles |

  Without an explicit `MaxKeys`, a wide alphabet or deep keys make the complete trie too large. When the pool size overflows `size_type`, a `static_assert` asks for a smaller `MaxKeys`.
- **Erase and handles.** `erase()` decrements the key counter on the key's path. The first node whose counter reaches zero is detached from its parent, and it and the rest of the path go on a free list. A handle to a freed node is *inert*: `prefix_count()` is 0, `is_terminal()` is `false`, and `advance()` returns `npos`. This holds until the next `insert()`, which may reuse the node, or `clear()`. A search can therefore erase words while it holds handles on its stack.
- **Lookups never fail loudly.** Keys that are too long or have symbols outside the alphabet simply are not found: `contains()` is `false`, `find()` is `npos`, and `erase()` returns `not_found`. Only `insert()` reports why a key is invalid.
- The empty key is valid and is stored at the root.
- `Value` must be default-constructible and copy-assignable. It is stored for every node, and only terminal nodes carry a meaningful payload.
- Construction value-initializes the whole pool, O(`static_node_capacity`). Place large tries in static storage.
- Not thread-safe. Concurrent readers are safe only when no thread writes.
