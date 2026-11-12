// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file disjoint_set.hpp
 * @brief Fixed-capacity disjoint-set union (union-find) over dense node indices.
 *
 * Use `disjoint_set<N>` to track a partition of the nodes `0 .. size() - 1` into groups that only
 * ever merge: connected components of a graph, Kruskal's minimum spanning tree, clustering of
 * sensor readings, or equivalence classes of identifiers. Nodes are plain indices, so they line up
 * one-to-one with vertex indices of a graph container. Every array lives inside the object; nothing
 * is allocated.
 *
 * `unite()` balances trees by rank and `find()` compresses paths, giving O(alpha(n)) amortized time
 * per operation, where alpha is the inverse Ackermann function (at most 4 for any practical n).
 * `find()` is iterative, so stack usage is constant.
 *
 * @code
 * castle::dsa::disjoint_set<6U> components;
 * components.unite(0U, 1U);
 * components.unite(1U, 2U);
 * bool linked = components.same_set(0U, 2U);       // true
 * castle::size_type groups = components.group_count(); // 4: {0,1,2} {3} {4} {5}
 * @endcode
 *
 * @see https://cp-algorithms.com/data_structures/disjoint_set_union.html
 */
#ifndef CASTLE_DSA_DISJOINT_SET_HPP
#define CASTLE_DSA_DISJOINT_SET_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/dsa/array_view.hpp"
#include "castle/error/status.hpp"
#include "castle/utility/swap.hpp"

#include <stdint.h>

namespace castle
{
namespace dsa
{

/**
 * @brief Union-find structure over up to `N` nodes with union by rank and path compression.
 * @tparam N Maximum number of nodes. Must be greater than zero.
 * @note Nodes are the dense indices `0 .. size() - 1`. `size()` defaults to `N` and can be changed
 * with `assign()`. Parent links, set sizes, and the node and group counters are stored in the
 * smallest unsigned type able to hold `N`, ranks in `uint8_t`.
 * @note The object is trivially copyable; copies are independent snapshots of the partition.
 * @warning Node arguments must be less than `size()`, not merely less than `N`. The check runs in
 * every build (it does not depend on `CASTLE_NO_CHECKS` or `NDEBUG`); a violating call returns a
 * neutral value (`npos`, `false`, `0`, or an empty view) and leaves the state untouched.
 */
template <size_type N>
class disjoint_set
{
    static_assert(N > 0U, "dsa::disjoint_set requires a positive capacity");

public:
    using size_type = castle::size_type;
    /** @brief Compact storage type for node indices and set sizes. */
    using index_type = meta::smallest_uint_t<N>;
    /** @brief Storage type for tree ranks; a rank never exceeds `log2(N)`. */
    using rank_type = uint8_t;

    /** @brief Compile-time node capacity. */
    static CASTLE_CONSTEXPR size_type static_capacity = N;
    /** @brief Value returned by index queries whose node argument is out of range. */
    static CASTLE_CONSTEXPR size_type npos = static_cast<size_type>(-1);

    /**
     * @brief Snapshot of every group of a `disjoint_set`, produced by `disjoint_set::groups()`.
     * @note Groups are numbered `0 .. size() - 1` in order of their smallest member; members of a
     * group are listed in ascending order. The snapshot does not follow later changes to the set.
     * @note Iterating the snapshot visits each group as an `array_view<const index_type>`.
     */
    class group_list
    {
    public:
        /** @brief Read-only view over the members of one group. */
        using value_type = array_view<CASTLE_CONST index_type>;

        /**
         * @brief Forward iterator over the groups of a snapshot.
         * @note Dereferencing yields the member view of the current group by value.
         */
        class const_iterator
        {
        public:
            /**
             * @brief Constructs an iterator positioned at `group` inside `owner`.
             * @param owner Snapshot being iterated.
             * @param group Zero-based group index; `owner->size()` is the end position.
             */
            const_iterator(CASTLE_CONST group_list* owner, size_type group) CASTLE_NOEXCEPT
                : owner_(owner), group_(group)
            {
            }

            /** @brief Returns the members of the current group. */
            value_type operator*() CASTLE_CONST CASTLE_NOEXCEPT { return (*owner_)[group_]; }

            /** @brief Advances to the next group. */
            const_iterator& operator++() CASTLE_NOEXCEPT
            {
                ++group_;
                return *this;
            }

            /** @brief Reports whether both iterators point at the same group of the same snapshot. */
            bool operator==(CASTLE_CONST const_iterator& other) CASTLE_CONST CASTLE_NOEXCEPT
            {
                return owner_ == other.owner_ && group_ == other.group_;
            }

            /** @brief Reports whether the iterators differ. */
            bool operator!=(CASTLE_CONST const_iterator& other) CASTLE_CONST CASTLE_NOEXCEPT
            {
                return !(*this == other);
            }

        private:
            CASTLE_CONST group_list* owner_;
            size_type group_;
        };

        /**
         * @brief Returns the number of groups.
         * @return Group count captured when the snapshot was taken.
         */
        size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return count_; }

        /**
         * @brief Reports whether the snapshot holds no groups.
         * @return `true` only for a set with zero nodes.
         */
        bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return count_ == 0U; }

        /**
         * @brief Returns the members of one group.
         * @param group Zero-based group index.
         * @return Ascending member indices, or an empty view when `group >= size()`.
         * @note Complexity: O(1). The view aliases the snapshot's storage.
         */
        value_type operator[](size_type group) CASTLE_CONST CASTLE_NOEXCEPT
        {
            if (group >= count_)
            {
                return value_type();
            }
            return value_type(members_ + offsets_[group],
                              static_cast<size_type>(offsets_[group + 1U] - offsets_[group]));
        }

        /**
         * @brief Returns the group index that contains `node`.
         * @param node Node index.
         * @return Group index in `0 .. size() - 1`, or `npos` when `node` is out of range.
         * @note Complexity: O(1).
         */
        size_type group_of(size_type node) CASTLE_CONST CASTLE_NOEXCEPT
        {
            if (node >= node_count_)
            {
                return npos;
            }
            return labels_[node];
        }

        /** @brief Returns an iterator to the first group. */
        const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return const_iterator(this, 0U); }

        /** @brief Returns an iterator one past the last group. */
        const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return const_iterator(this, count_); }

    private:
        friend class disjoint_set;

        /**
         * @brief Builds the snapshot by bucketing every node under its group.
         * @param set Source set; its paths are fully compressed as a side effect.
         * @note Complexity: O(N) for value-initialization plus O(n alpha(n)) for the scan.
         */
        explicit group_list(disjoint_set& set) CASTLE_NOEXCEPT
            : members_{}, offsets_{}, labels_{}, node_count_(set.node_count_), count_(0U)
        {
            CASTLE_CONST index_type unlabeled = static_cast<index_type>(N);
            for (size_type i = 0U; i < node_count_; ++i)
            {
                labels_[i] = unlabeled;
            }

            // A root's label doubles as its group's number; ascending scan numbers groups by smallest member.
            for (size_type i = 0U; i < node_count_; ++i)
            {
                CASTLE_CONST size_type root = set.root_of(i);
                if (labels_[root] == unlabeled)
                {
                    labels_[root] = static_cast<index_type>(count_);
                    ++count_;
                }
                labels_[i] = labels_[root];
            }

            // Counting sort: offsets_[g + 1] = size of group g, then prefix sums give group starts.
            for (size_type i = 0U; i < node_count_; ++i)
            {
                ++offsets_[labels_[i] + 1U];
            }
            for (size_type g = 1U; g <= count_; ++g)
            {
                offsets_[g] = static_cast<index_type>(offsets_[g] + offsets_[g - 1U]);
            }

            // Scatter using offsets_[g] as the write cursor, which leaves it at the start of group g + 1.
            for (size_type i = 0U; i < node_count_; ++i)
            {
                members_[offsets_[labels_[i]]] = static_cast<index_type>(i);
                ++offsets_[labels_[i]];
            }
            for (size_type g = count_; g > 0U; --g)
            {
                offsets_[g] = offsets_[g - 1U];
            }
            offsets_[0U] = 0U;
        }

        index_type members_[N];
        index_type offsets_[N + 1U];
        index_type labels_[N];
        index_type node_count_;
        index_type count_;
    };

    /**
     * @brief Constructs `nodes` singleton groups.
     * @param nodes Number of nodes, `0 .. N`. Defaults to `N`.
     * @note Complexity: O(N).
     * @warning `nodes > N` reports through `CASTLE_ASSERT` and is clamped to `N`.
     */
    explicit disjoint_set(size_type nodes = N) CASTLE_NOEXCEPT
        : parent_{}, set_size_{}, rank_{}, node_count_(0U), group_count_(0U)
    {
        CASTLE_ASSERT(nodes <= N, // LCOV_EXCL_BR_LINE
                      CASTLE_ERROR_GENERIC("castle::dsa::disjoint_set: node count exceeds capacity"));
        initialize(nodes <= N ? nodes : N); // LCOV_EXCL_BR_LINE
    }

    /**
     * @brief Replaces the partition with `nodes` singleton groups.
     * @param nodes New number of nodes, `0 .. N`.
     * @return `status::ok`, or `status::out_of_range` when `nodes > N` (the set is left unchanged).
     * @note Complexity: O(nodes).
     */
    status assign(size_type nodes) CASTLE_NOEXCEPT
    {
        if (nodes > N)
        {
            return status::out_of_range;
        }
        initialize(nodes);
        return status::ok;
    }

    /**
     * @brief Returns the number of nodes.
     * @return Node count; valid node indices are `0 .. size() - 1`.
     */
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return node_count_; }

    /**
     * @brief Returns the compile-time node capacity.
     * @return `N`.
     */
    CASTLE_CONSTEXPR size_type capacity() CASTLE_CONST CASTLE_NOEXCEPT { return N; }

    /**
     * @brief Reports whether the set has no nodes.
     * @return `true` when `size() == 0`.
     */
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return node_count_ == 0U; }

    /**
     * @brief Returns the number of disjoint groups.
     * @return Group count in O(1).
     */
    size_type group_count() CASTLE_CONST CASTLE_NOEXCEPT { return group_count_; }

    /**
     * @brief Returns the representative (root) of the group containing `node`.
     * @param node Node index.
     * @return Root node index, or `npos` when `node` is out of range.
     * @note Compresses the path from `node` to its root. Amortized O(alpha(n)).
     * @warning The representative of a group may change after `unite()` or `reset(node)`.
     */
    size_type find(size_type node) CASTLE_NOEXCEPT
    {
        if (node >= node_count_)
        {
            return npos;
        }
        return root_of(node);
    }

    /**
     * @brief Merges the groups containing `lhs` and `rhs`.
     * @param lhs First node index.
     * @param rhs Second node index.
     * @return `true` when two different groups were merged; `false` when both nodes already shared
     * a group or an index is out of range.
     * @note The lower-rank root is attached below the higher-rank root. Amortized O(alpha(n)).
     */
    bool unite(size_type lhs, size_type rhs) CASTLE_NOEXCEPT
    {
        if ((lhs >= node_count_) || (rhs >= node_count_))
        {
            return false;
        }
        size_type high = root_of(lhs);
        size_type low = root_of(rhs);
        if (high == low)
        {
            return false;
        }
        if (rank_[high] < rank_[low])
        {
            castle::swap(high, low);
        }

        parent_[low] = static_cast<index_type>(high);
        set_size_[high] = static_cast<index_type>(set_size_[high] + set_size_[low]);
        if (rank_[high] == rank_[low])
        {
            ++rank_[high];
        }
        --group_count_;
        return true;
    }

    /**
     * @brief Reports whether two nodes belong to the same group.
     * @param lhs First node index.
     * @param rhs Second node index.
     * @return `true` when both share a root; `false` otherwise or when an index is out of range.
     * @note Amortized O(alpha(n)).
     */
    bool same_set(size_type lhs, size_type rhs) CASTLE_NOEXCEPT
    {
        if ((lhs >= node_count_) || (rhs >= node_count_))
        {
            return false;
        }
        return root_of(lhs) == root_of(rhs);
    }

    /**
     * @brief Returns the number of nodes in the group containing `node`.
     * @param node Node index.
     * @return Group size (at least 1), or 0 when `node` is out of range.
     * @note Amortized O(alpha(n)).
     */
    size_type group_size(size_type node) CASTLE_NOEXCEPT
    {
        if (node >= node_count_)
        {
            return 0U;
        }
        return set_size_[root_of(node)];
    }

    /**
     * @brief Detaches `node` into its own singleton group (own parent, size 1, rank 0).
     * @param node Node index.
     * @note The remaining members stay in one group. Nodes linked through `node` are re-linked to
     * the group's root; when `node` was the root, its first child becomes the new root.
     * @note Complexity: O(size()) because the children of `node` are found by a linear scan. Does
     * nothing when `node` is out of range.
     */
    void reset(size_type node) CASTLE_NOEXCEPT
    {
        if (node >= node_count_)
        {
            return;
        }
        CASTLE_CONST size_type root = root_of(node);
        CASTLE_CONST size_type remaining = static_cast<size_type>(set_size_[root] - 1U);
        if (remaining != 0U)
        {
            size_type new_root = (node == root) ? npos : root;
            for (size_type i = 0U; i < node_count_; ++i)
            {
                if ((i != node) && (parent_[i] == node))
                {
                    if (new_root == npos)
                    {
                        new_root = i;
                    }
                    parent_[i] = static_cast<index_type>(new_root);
                }
            }
            if (new_root != root)
            {
                // The heir inherits the old root's rank: the merged subtrees are no taller than before.
                rank_[new_root] = rank_[node];
            }
            set_size_[new_root] = static_cast<index_type>(remaining);
            ++group_count_;
        }
        make_singleton(node);
    }

    /**
     * @brief Returns every node to its own singleton group, keeping `size()`.
     * @note Complexity: O(size()).
     */
    void reset() CASTLE_NOEXCEPT
    {
        initialize(node_count_);
    }

    /**
     * @brief Captures every group and its members.
     * @return Snapshot listing `group_count()` groups ordered by smallest member.
     * @note Fully compresses all paths. Complexity: O(N) plus O(n alpha(n)).
     * @note The snapshot holds `3 * N + 1` indices, so it is sized like the set itself; place it
     * where that footprint is acceptable.
     */
    group_list groups() CASTLE_NOEXCEPT
    {
        return group_list(*this);
    }

private:
    /**
     * @brief Resets the first `nodes` nodes to singletons and sets the node count.
     * @param nodes Node count, already validated against `N`.
     */
    void initialize(size_type nodes) CASTLE_NOEXCEPT
    {
        node_count_ = static_cast<index_type>(nodes);
        group_count_ = static_cast<index_type>(nodes);
        for (size_type i = 0U; i < nodes; ++i)
        {
            make_singleton(i);
        }
    }

    /**
     * @brief Makes `node` its own root with size 1 and rank 0.
     * @param node Valid node index.
     */
    void make_singleton(size_type node) CASTLE_NOEXCEPT
    {
        parent_[node] = static_cast<index_type>(node);
        set_size_[node] = 1U;
        rank_[node] = 0U;
    }

    /**
     * @brief Finds the root of `node` and points every node on the path directly at it.
     * @param node Valid node index.
     * @return Root node index.
     */
    size_type root_of(size_type node) CASTLE_NOEXCEPT
    {
        size_type root = node;
        while (parent_[root] != root)
        {
            root = parent_[root];
        }
        while (parent_[node] != root)
        {
            CASTLE_CONST size_type next = parent_[node];
            parent_[node] = static_cast<index_type>(root);
            node = next;
        }
        return root;
    }

    index_type parent_[N];
    index_type set_size_[N];
    rank_type rank_[N];
    index_type node_count_;
    index_type group_count_;
};

} // namespace dsa
} // namespace castle

#endif // CASTLE_DSA_DISJOINT_SET_HPP
