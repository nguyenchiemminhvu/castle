// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file trie.hpp
 * @brief Fixed-capacity prefix tree bounded by alphabet width, key depth, and key count.
 *
 * Use `trie<Alphabet, MaxDepth, MaxKeys, Value>` for dictionaries, prefix matching, command
 * tables, word games, and board searches. Every node has one child slot per alphabet symbol, so
 * stepping from a node to a child is O(1). The node pool is embedded in the object and sized at
 * compile time from the three bounds, so any set of up to `MaxKeys` keys of up to `MaxDepth`
 * symbols always fits: insertion never fails for lack of nodes.
 *
 * Erasing a key frees the nodes no other key uses. A handle to a freed node becomes inert: it
 * reports no keys, is not terminal, and `advance()` from it returns `npos`. This makes the trie
 * suitable for searches that remove words as they are found (LeetCode 212, Word Search II).
 *
 * @code
 * castle::dsa::trie<castle::dsa::lowercase_alphabet, 8U, 16U> words;
 * words.insert("car");
 * words.insert("cart");
 * bool has = words.contains("car");             // true
 * castle::size_type n = words.prefix_count("ca"); // 2
 * @endcode
 *
 * @see https://cp-algorithms.com/string/aho_corasick.html#construction-of-the-trie
 */
#ifndef CASTLE_DSA_TRIE_HPP
#define CASTLE_DSA_TRIE_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/status.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/dsa/array.hpp"
#include "castle/dsa/string_view.hpp"

#include <stdint.h>

namespace castle
{
namespace dsa
{

/**
 * @brief Alphabet of `Size` consecutive symbol values starting at `First`.
 * @tparam Symbol Non-bool integral symbol type.
 * @tparam First First symbol of the alphabet; it maps to child index 0.
 * @tparam Size Number of symbols, which is also the child width of every trie node.
 * @note A trie alphabet is any type providing `symbol_type`, a `size` constant, and a static
 * `index_of(symbol_type)` that returns the child index in `0 .. size - 1`, or any value `>= size`
 * for a symbol outside the alphabet. Write a custom one for non-contiguous symbol sets.
 */
template <typename Symbol, Symbol First, size_type Size>
struct contiguous_alphabet
{
    static_assert(meta::is_integral<Symbol>::value && !meta::is_same<Symbol, bool>::value,
                  "dsa::contiguous_alphabet symbols must be non-bool integral types");
    static_assert(Size > 0U, "dsa::contiguous_alphabet requires at least one symbol");

    /** @brief Symbol type accepted by the trie. */
    using symbol_type = Symbol;

    /** @brief Number of symbols; the child width of each trie node. */
    static CASTLE_CONSTEXPR size_type size = Size;

    /**
     * @brief Maps a symbol to its child index.
     * @param symbol Symbol to map.
     * @return `symbol - First` when the symbol is inside the alphabet; a value `>= Size` otherwise.
     * @note The subtraction is done in the unsigned counterpart of `Symbol`, so symbols below
     * `First` wrap to large values instead of overflowing.
     */
    static CASTLE_CONSTEXPR size_type index_of(Symbol symbol) CASTLE_NOEXCEPT
    {
        using unsigned_type = meta::make_unsigned_t<Symbol>;
        return static_cast<size_type>(
            static_cast<unsigned_type>(static_cast<unsigned_type>(symbol) - static_cast<unsigned_type>(First)));
    }
};

/** @brief The 26 lowercase English letters `'a'` .. `'z'`. */
using lowercase_alphabet = contiguous_alphabet<char, 'a', 26U>;

/** @brief The 10 decimal digits `'0'` .. `'9'`. */
using digit_alphabet = contiguous_alphabet<char, '0', 10U>;

namespace detail
{

/** @brief Returns `lhs + rhs`, or `size_type(-1)` when the sum overflows. */
CASTLE_CONSTEXPR size_type trie_saturating_add(size_type lhs, size_type rhs) CASTLE_NOEXCEPT
{
    return (lhs > static_cast<size_type>(-1) - rhs) ? static_cast<size_type>(-1) : lhs + rhs;
}

/**
 * @brief Upper bound of trie nodes, root included, for `keys` keys of at most `depth` symbols.
 * @param width Alphabet size.
 * @param depth Maximum key length.
 * @param keys Maximum number of stored keys.
 * @return `1 + sum(min(width^d, keys))` for `d = 1 .. depth`, saturated at `size_type(-1)`.
 * @note Depth `d` holds at most `width^d` distinct prefixes, and every node lies on the path of at
 * least one stored key, so it also holds at most `keys` nodes.
 */
CASTLE_CONSTEXPR size_type trie_node_capacity(size_type width, size_type depth, size_type keys) CASTLE_NOEXCEPT
{
    size_type total = 1U;
    size_type layer = 1U;
    for (size_type d = 0U; d < depth; ++d)
    {
        layer = (layer > keys / width) ? keys : layer * width;
        total = trie_saturating_add(total, layer);
    }
    return total;
}

/**
 * @brief Number of distinct keys of length `0 .. depth` over `width` symbols, saturated.
 * @return `sum(width^d)` for `d = 0 .. depth`; with this many keys, the trie is complete.
 */
CASTLE_CONSTEXPR size_type trie_full_key_count(size_type width, size_type depth) CASTLE_NOEXCEPT
{
    return trie_node_capacity(width, depth, static_cast<size_type>(-1));
}

} // namespace detail

/**
 * @brief Fixed-capacity trie with a dense child table per node.
 * @tparam Alphabet Symbol-to-index policy, for example `lowercase_alphabet`, `digit_alphabet`, or
 * any `contiguous_alphabet`. `Alphabet::size` is the width of every node.
 * @tparam MaxDepth Maximum key length in symbols, which is the number of node layers below the root.
 * @tparam MaxKeys Maximum number of stored keys. Defaults to every key the alphabet can form up to
 * `MaxDepth` symbols, which makes the trie complete; set it explicitly for wide alphabets or deep
 * keys, where the complete trie is too large.
 * @tparam Value Payload stored with each key. Must be default-constructible and copy-assignable.
 *
 * @note The node pool holds `static_node_capacity = 1 + sum(min(width^d, MaxKeys))` nodes for
 * `d = 1 .. MaxDepth`. Each node stores `width` child handles, a key counter, a terminal flag,
 * and one `Value`.
 * @note Keys may be empty; the empty key is stored at the root.
 * @warning Node handles are invalidated by `clear()`, and a handle to a node freed by `erase()`
 * may refer to a different prefix after a later `insert()`. Between those calls, a handle to a
 * freed node is inert: `prefix_count()` is 0, `is_terminal()` is false, and `advance()` returns
 * `npos`.
 */
template <typename Alphabet,
          size_type MaxDepth,
          size_type MaxKeys = detail::trie_full_key_count(Alphabet::size, MaxDepth),
          typename Value = uint32_t>
class trie
{
    static_assert(Alphabet::size > 0U, "dsa::trie requires a non-empty alphabet");
    static_assert(MaxKeys > 0U, "dsa::trie requires a positive key capacity");

public:
    using alphabet_type = Alphabet;
    using symbol_type = typename Alphabet::symbol_type;
    /** @brief Key view; accepts null-terminated strings and `(pointer, length)` views. */
    using key_type = basic_string_view<symbol_type>;
    using value_type = Value;
    using size_type = castle::size_type;

    /** @brief Child width of every node, `Alphabet::size`. */
    static CASTLE_CONSTEXPR size_type static_width = Alphabet::size;
    /** @brief Maximum key length. */
    static CASTLE_CONSTEXPR size_type static_max_depth = MaxDepth;
    /** @brief Maximum number of stored keys. */
    static CASTLE_CONSTEXPR size_type static_capacity = MaxKeys;
    /** @brief Number of nodes in the embedded pool, root included. */
    static CASTLE_CONSTEXPR size_type static_node_capacity =
        detail::trie_node_capacity(Alphabet::size, MaxDepth, MaxKeys);

    static_assert(static_node_capacity != static_cast<size_type>(-1),
                  "dsa::trie node pool is too large: lower MaxKeys or MaxDepth");

    /** @brief Node index type; the smallest unsigned type able to hold `static_node_capacity`. */
    using node_handle = meta::smallest_uint_t<static_node_capacity>;
    /** @brief Key counter type; the smallest unsigned type able to hold `MaxKeys`. */
    using count_type = meta::smallest_uint_t<MaxKeys>;

    /** @brief Handle returned when a prefix or child does not exist. */
    static CASTLE_CONSTEXPR node_handle npos = static_cast<node_handle>(static_node_capacity);

private:
    struct node
    {
        array<node_handle, Alphabet::size> child; // child[0] links the free list while the node is free
        count_type keys;                           // keys stored at or below this node; 0 when free
        bool terminal;
        Value value;
    };

    static_assert(static_node_capacity <= static_cast<size_type>(-1) / sizeof(node),
                  "dsa::trie node pool is too large: lower MaxKeys or MaxDepth");

public:
    /**
     * @brief Constructs an empty trie.
     * @note Complexity: O(static_node_capacity) for value-initializing the pool.
     */
    trie() CASTLE_NOEXCEPT
        : nodes_{}
        , free_head_(npos)
        , next_unused_(1U)
        , live_nodes_(1U)
    {
        reset_node(root());
    }

    /**
     * @brief Inserts a key with its payload.
     * @param key Key symbols.
     * @param value Payload stored with the key.
     * @return `status::ok`; `status::already_exists` when the key is present (its payload is kept);
     * `status::full` when `size() == MaxKeys`; `status::out_of_range` when the key is longer than
     * `MaxDepth`; `status::invalid_argument` when a symbol is outside the alphabet.
     * @note Complexity: O(key.size()). A rejected insertion leaves the trie unchanged.
     */
    status insert(key_type key, CASTLE_CONST Value& value = Value()) CASTLE_NOEXCEPT
    {
        if (key.size() > MaxDepth)
        {
            return status::out_of_range;
        }
        for (size_type i = 0U; i < key.size(); ++i)
        {
            if (Alphabet::index_of(key[i]) >= Alphabet::size)
            {
                return status::invalid_argument;
            }
        }

        CASTLE_CONST node_handle existing = find(key);
        if ((existing != npos) && nodes_[existing].terminal)
        {
            return status::already_exists;
        }
        if (size() == MaxKeys)
        {
            return status::full;
        }

        // The pool bound guarantees a free node for every missing prefix.
        node_handle current = root();
        ++nodes_[current].keys;
        for (size_type i = 0U; i < key.size(); ++i)
        {
            node_handle& slot = nodes_[current].child[Alphabet::index_of(key[i])];
            if (slot == npos)
            {
                slot = allocate();
            }
            current = slot;
            ++nodes_[current].keys;
        }
        nodes_[current].terminal = true;
        nodes_[current].value = value;
        return status::ok;
    }

    /**
     * @brief Erases a key and frees the nodes no remaining key uses.
     * @param key Key symbols.
     * @return `status::ok`, or `status::not_found` when the key is not stored.
     * @note Complexity: O(key.size()).
     */
    status erase(key_type key) CASTLE_NOEXCEPT
    {
        CASTLE_CONST node_handle target = find(key);
        if ((target == npos) || !nodes_[target].terminal)
        {
            return status::not_found;
        }
        nodes_[target].terminal = false;
        nodes_[target].value = Value();

        node_handle current = root();
        --nodes_[current].keys;
        for (size_type i = 0U; i < key.size(); ++i)
        {
            node_handle& slot = nodes_[current].child[Alphabet::index_of(key[i])];
            CASTLE_CONST node_handle next = slot;
            --nodes_[next].keys;
            if (nodes_[next].keys == 0U)
            {
                // Nothing below `next` is stored any more: detach it and free the rest of the path.
                slot = npos;
                release_path(next, key, i + 1U);
                break;
            }
            current = next;
        }
        return status::ok;
    }

    /**
     * @brief Reports whether the exact key is stored.
     * @note Complexity: O(key.size()).
     */
    bool contains(key_type key) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return is_terminal(find(key));
    }

    /**
     * @brief Reports whether at least one stored key starts with `prefix`.
     * @note Complexity: O(prefix.size()). The empty prefix matches any non-empty trie.
     */
    bool contains_prefix(key_type prefix) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return prefix_count(find(prefix)) != 0U;
    }

    /**
     * @brief Returns the number of stored keys that start with `prefix`, the prefix itself included.
     * @note Complexity: O(prefix.size()).
     */
    size_type prefix_count(key_type prefix) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return prefix_count(find(prefix));
    }

    /**
     * @brief Returns the number of stored keys at or below a node.
     * @param handle Node handle; `npos` and freed nodes yield 0.
     * @note Complexity: O(1).
     */
    size_type prefix_count(node_handle handle) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (handle < static_node_capacity) ? static_cast<size_type>(nodes_[handle].keys) : 0U;
    }

    /**
     * @brief Returns the node that represents `prefix`.
     * @return Node handle, or `npos` when no stored key starts with `prefix`.
     * @note Complexity: O(prefix.size()).
     */
    node_handle find(key_type prefix) CASTLE_CONST CASTLE_NOEXCEPT
    {
        node_handle current = root();
        for (size_type i = 0U; (i < prefix.size()) && (current != npos); ++i)
        {
            current = advance(current, prefix[i]);
        }
        return current;
    }

    /** @brief Returns the root handle; the root represents the empty prefix. */
    CASTLE_CONSTEXPR node_handle root() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<node_handle>(0U);
    }

    /**
     * @brief Steps from a node to the child reached by `symbol`.
     * @param from Node handle; `npos` is accepted, so calls can be chained.
     * @param symbol Next key symbol.
     * @return Child handle, or `npos` when `from` is `npos` or freed, the symbol is outside the
     * alphabet, or no stored key continues with `symbol`.
     * @note Complexity: O(1).
     */
    node_handle advance(node_handle from, symbol_type symbol) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type index = Alphabet::index_of(symbol);
        if ((from >= static_node_capacity) || (nodes_[from].keys == 0U) || (index >= Alphabet::size))
        {
            return npos;
        }
        return nodes_[from].child[index];
    }

    /**
     * @brief Reports whether a node ends a stored key.
     * @param handle Node handle; `npos` and freed nodes yield `false`.
     */
    bool is_terminal(node_handle handle) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (handle < static_node_capacity) && nodes_[handle].terminal;
    }

    /**
     * @brief Copies the payload of the key ending at `handle`.
     * @param handle Node handle, typically from `find()` or `advance()`.
     * @param out Receives the payload on success; untouched otherwise.
     * @return `status::ok`, or `status::not_found` when `handle` does not end a stored key.
     */
    status try_value(node_handle handle, Value& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (!is_terminal(handle))
        {
            return status::not_found;
        }
        out = nodes_[handle].value;
        return status::ok;
    }

    /**
     * @brief Removes every key and returns all nodes to the pool.
     * @note Complexity: O(width). Invalidates every node handle.
     */
    void clear() CASTLE_NOEXCEPT
    {
        reset_node(root());
        free_head_ = npos;
        next_unused_ = 1U;
        live_nodes_ = 1U;
    }

    /** @brief Returns the number of stored keys. */
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<size_type>(nodes_[root()].keys);
    }

    /** @brief Returns `MaxKeys`. */
    CASTLE_CONSTEXPR size_type capacity() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return MaxKeys;
    }

    /** @brief Returns `MaxDepth`, the longest accepted key. */
    CASTLE_CONSTEXPR size_type max_depth() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return MaxDepth;
    }

    /** @brief Reports whether no key is stored. */
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return size() == 0U;
    }

    /** @brief Reports whether `MaxKeys` keys are stored. */
    bool full() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return size() == MaxKeys;
    }

    /** @brief Returns the number of nodes in use, root included. */
    size_type node_count() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return live_nodes_;
    }

    /** @brief Returns `static_node_capacity`. */
    CASTLE_CONSTEXPR size_type node_capacity() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_node_capacity;
    }

private:
    /** @brief Makes `handle` an empty, non-terminal node without children. */
    void reset_node(node_handle handle) CASTLE_NOEXCEPT
    {
        node& target = nodes_[handle];
        for (size_type i = 0U; i < Alphabet::size; ++i)
        {
            target.child[i] = npos;
        }
        target.keys = 0U;
        target.terminal = false;
        target.value = Value();
    }

    /** @brief Takes a node from the free list, or else from the never-used tail of the pool. */
    node_handle allocate() CASTLE_NOEXCEPT
    {
        node_handle handle = free_head_;
        if (handle != npos)
        {
            free_head_ = nodes_[handle].child[0U];
        }
        else
        {
            handle = next_unused_;
            ++next_unused_;
        }
        reset_node(handle);
        ++live_nodes_;
        return handle;
    }

    /**
     * @brief Frees `first` and its descendants along the rest of `key`.
     * @param first Detached node at depth `depth`.
     * @param key Erased key whose remaining symbols select the nodes to free.
     * @param depth Depth of `first`.
     */
    void release_path(node_handle first, key_type key, size_type depth) CASTLE_NOEXCEPT
    {
        node_handle current = first;
        while (current != npos)
        {
            CASTLE_CONST node_handle next =
                (depth < key.size()) ? nodes_[current].child[Alphabet::index_of(key[depth])] : npos;
            nodes_[current].keys = 0U;
            nodes_[current].terminal = false;
            nodes_[current].child[0U] = free_head_;
            free_head_ = current;
            --live_nodes_;
            current = next;
            ++depth;
        }
    }

    array<node, static_node_capacity> nodes_;
    node_handle free_head_;
    node_handle next_unused_;
    node_handle live_nodes_;
};

} // namespace dsa
} // namespace castle

#endif // CASTLE_DSA_TRIE_HPP
