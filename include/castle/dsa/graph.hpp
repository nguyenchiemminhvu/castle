// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file graph.hpp
 * @brief Fixed-capacity adjacency-list graphs and allocation-free graph algorithms.
 *
 * The header has two independent halves:
 *
 * - **Data structure** - `basic_graph<N, M, Directed, Weight>` stores up to `N` nodes and `M` edges
 *   as insertion-ordered adjacency lists threaded through fixed arrays. It only knows how to add,
 *   find, remove, and enumerate edges. The aliases `undirected_graph`, `directed_graph`,
 *   `undirected_weighted_graph`, and `directed_weighted_graph` name the four supported kinds.
 * - **Algorithms** - free function templates in `castle::dsa::graph_algorithms` that take any of the
 *   four graph kinds: BFS/DFS traversal, reachability, connected and strongly connected components
 *   (union-find, Tarjan), cycle detection, topological sort (Kahn), shortest paths (BFS, Dijkstra,
 *   Bellman-Ford, Floyd-Warshall), minimum spanning tree (Kruskal), bridges and articulation points
 *   (Tarjan low-link), greedy (Welsh-Powell) and bipartite coloring.
 *
 * Nodes are the dense indices `0 .. size() - 1`; `size()` is chosen at runtime up to the capacity
 * `N`. Nothing is allocated and no algorithm recurses: scratch arrays sized by `N` or `M` live on
 * the stack of the algorithm call, results are written to caller-owned fixed-capacity objects.
 *
 * @code
 * using network = castle::dsa::undirected_weighted_graph<5U, 8U, uint16_t>;
 * network g(5U, {{0U, 1U, 4U}, {1U, 2U, 1U}, {0U, 2U, 7U}, {2U, 3U, 2U}});
 *
 * namespace ga = castle::dsa::graph_algorithms;
 * uint16_t cost = ga::distance(g, 0U, 3U);              // 7 via 0-1-2-3
 * bool linked = ga::connected(g, 0U, 4U);               // false: node 4 is isolated
 * ga::edge_list<network> tree;
 * uint16_t wiring = ga::minimum_spanning_tree(g, tree); // 7: edges 1-2, 2-3, 0-1
 * @endcode
 */
#ifndef CASTLE_DSA_GRAPH_HPP
#define CASTLE_DSA_GRAPH_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/type_ranges.hpp"
#include "castle/core/types.hpp"
#include "castle/dsa/algorithm.hpp"
#include "castle/dsa/array_view.hpp"
#include "castle/dsa/disjoint_set.hpp"
#include "castle/dsa/grid.hpp"
#include "castle/dsa/initializer_list.hpp"
#include "castle/dsa/vector.hpp"
#include "castle/error/status.hpp"
#include "castle/utility/bitset.hpp"
#include "castle/utility/swap.hpp"

#include <stdint.h>

namespace castle
{
namespace dsa
{

/**
 * @brief Edge description used to build graphs and to report edges from algorithms.
 * @tparam Weight Arithmetic weight type.
 * @note Aggregate: `{from, to, weight}`.
 */
template <typename Weight>
struct graph_edge
{
    size_type from; /**< Source node (either end for undirected graphs). */
    size_type to;   /**< Target node. */
    Weight weight;  /**< Edge weight. */
};

/**
 * @brief Unweighted edge description: `{from, to}`.
 */
template <>
struct graph_edge<void>
{
    size_type from; /**< Source node (either end for undirected graphs). */
    size_type to;   /**< Target node. */
};

namespace detail
{

/**
 * @brief Per-arc weight storage of a weighted graph.
 * @tparam Weight Arithmetic weight type.
 * @tparam Arcs Number of arc slots.
 */
template <typename Weight, size_type Arcs>
class graph_weight_store
{
public:
    using weight_type = Weight;

    graph_weight_store() CASTLE_NOEXCEPT : weights_{} {}

    weight_type stored_weight(size_type arc) CASTLE_CONST CASTLE_NOEXCEPT { return weights_[arc]; }

    void store_weight(size_type arc, weight_type weight) CASTLE_NOEXCEPT { weights_[arc] = weight; }

private:
    Weight weights_[Arcs];
};

/**
 * @brief Empty weight storage of an unweighted graph: every arc weighs 1.
 * @tparam Arcs Number of arc slots (unused).
 */
template <size_type Arcs>
class graph_weight_store<void, Arcs>
{
public:
    using weight_type = size_type;

    weight_type stored_weight(size_type) CASTLE_CONST CASTLE_NOEXCEPT { return 1U; }

    void store_weight(size_type, weight_type) CASTLE_NOEXCEPT {}
};

} // namespace detail

/**
 * @brief Fixed-capacity graph stored as adjacency lists over dense node indices.
 * @tparam N Maximum number of nodes. Must be greater than zero.
 * @tparam M Maximum number of edges. Must be greater than zero. An undirected edge counts once.
 * @tparam Directed `true` for a directed graph, `false` for an undirected graph.
 * @tparam Weight Arithmetic weight type, or `void` for an unweighted graph whose edges weigh 1.
 * @note Each node owns a singly linked list of outgoing arcs threaded through fixed arrays, so
 * memory is O(N + M) and neighbors are enumerated in edge insertion order. An undirected edge is
 * stored as two arcs (one per end); a directed edge as one arc.
 * @note Self-loops are accepted. Parallel edges are rejected (`status::already_exists`).
 * @note The object is trivially copyable; copies are independent.
 * @warning Node arguments must be less than `size()`. Edge mutators report violations through
 * `status`; queries return a neutral value (`false`, `0`, `npos`, an empty range). The arc-level
 * accessors (`next_arc()`, `target()`, `weight()`, `edge_index()`) expect a live arc handle and do
 * not check it.
 */
template <size_type N, size_type M, bool Directed, typename Weight = void>
class basic_graph : private detail::graph_weight_store<Weight, (Directed ? M : 2U * M)>
{
    static_assert(N > 0U, "dsa::basic_graph requires a positive node capacity");
    static_assert(M > 0U, "dsa::basic_graph requires a positive edge capacity");
    static_assert(meta::is_void<Weight>::value || meta::is_arithmetic<Weight>::value,
                  "dsa::basic_graph weight must be void (unweighted) or an arithmetic type");

    using weight_store = detail::graph_weight_store<Weight, (Directed ? M : 2U * M)>;

public:
    using size_type = castle::size_type;
    /** @brief Compact storage type for node indices; holds `N` as the "no node" marker. */
    using index_type = meta::smallest_uint_t<N>;
    /** @brief Compact storage type for arc indices; holds `static_arc_capacity` as the "no arc" marker. */
    using arc_index_type = meta::smallest_uint_t<(Directed ? M : 2U * M)>;
    /** @brief Edge weight type: `Weight`, or `size_type` (always 1) for unweighted graphs. */
    using weight_type = typename weight_store::weight_type;
    /** @brief Edge description accepted by `add_edge()` and reported by algorithms. */
    using edge_type = graph_edge<Weight>;

    /** @brief Compile-time node capacity. */
    static CASTLE_CONSTEXPR size_type static_capacity = N;
    /** @brief Compile-time edge capacity. */
    static CASTLE_CONSTEXPR size_type static_edge_capacity = M;
    /** @brief Compile-time arc capacity: `M` for directed, `2 * M` for undirected graphs. */
    static CASTLE_CONSTEXPR size_type static_arc_capacity = Directed ? M : 2U * M;
    /** @brief `true` for directed graphs. */
    static CASTLE_CONSTEXPR bool directed = Directed;
    /** @brief `true` when edges carry a stored weight. */
    static CASTLE_CONSTEXPR bool weighted = !meta::is_void<Weight>::value;
    /** @brief Handle returned when no node or arc exists. */
    static CASTLE_CONSTEXPR size_type npos = static_cast<size_type>(-1);

    /** @brief One entry of an adjacency list. */
    struct neighbor
    {
        size_type node;     /**< Adjacent node. */
        weight_type weight; /**< Weight of the connecting arc (1 for unweighted graphs). */
    };

    /**
     * @brief Forward iterator over the adjacency list of one node.
     * @note Dereferencing yields a `neighbor` by value.
     */
    class neighbor_iterator
    {
    public:
        /**
         * @brief Constructs an iterator positioned at `arc`.
         * @param owner Graph being iterated.
         * @param arc Arc handle, or `npos` for the end position.
         */
        neighbor_iterator(CASTLE_CONST basic_graph* owner, size_type arc) CASTLE_NOEXCEPT
            : owner_(owner), arc_(arc)
        {
        }

        /** @brief Returns the neighbor reached through the current arc. */
        neighbor operator*() CASTLE_CONST CASTLE_NOEXCEPT
        {
            return neighbor{owner_->target(arc_), owner_->weight(arc_)};
        }

        /** @brief Advances to the next arc of the same node. */
        neighbor_iterator& operator++() CASTLE_NOEXCEPT
        {
            arc_ = owner_->next_arc(arc_);
            return *this;
        }

        /** @brief Returns the arc handle of the current position (`npos` at the end). */
        size_type arc() CASTLE_CONST CASTLE_NOEXCEPT { return arc_; }

        /** @brief Reports whether both iterators point at the same arc of the same graph. */
        bool operator==(CASTLE_CONST neighbor_iterator& other) CASTLE_CONST CASTLE_NOEXCEPT
        {
            return owner_ == other.owner_ && arc_ == other.arc_;
        }

        /** @brief Reports whether the iterators differ. */
        bool operator!=(CASTLE_CONST neighbor_iterator& other) CASTLE_CONST CASTLE_NOEXCEPT
        {
            return !(*this == other);
        }

    private:
        CASTLE_CONST basic_graph* owner_;
        size_type arc_;
    };

    /** @brief Range over the adjacency list of one node, usable in range-`for`. */
    class neighbor_range
    {
    public:
        /**
         * @brief Constructs a range starting at `first`.
         * @param owner Graph being iterated.
         * @param first First arc handle, or `npos` for an empty range.
         */
        neighbor_range(CASTLE_CONST basic_graph* owner, size_type first) CASTLE_NOEXCEPT
            : owner_(owner), first_(first)
        {
        }

        /** @brief Returns an iterator to the first neighbor. */
        neighbor_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return neighbor_iterator(owner_, first_); }

        /** @brief Returns the end iterator. */
        neighbor_iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return neighbor_iterator(owner_, npos); }

        /** @brief Reports whether the node has no outgoing arcs. */
        bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return first_ == npos; }

    private:
        CASTLE_CONST basic_graph* owner_;
        size_type first_;
    };

    /**
     * @brief Constructs a graph with `nodes` nodes and no edges.
     * @param nodes Number of nodes, `0 .. N`. Defaults to `N`.
     * @note Complexity: O(N + M).
     * @warning `nodes > N` reports through `CASTLE_ASSERT` and is clamped to `N`.
     */
    explicit basic_graph(size_type nodes = N) CASTLE_NOEXCEPT
        : weight_store(), head_{}, next_{}, target_{}, free_slot_(no_arc), used_slots_(0U), edge_count_(0U),
          node_count_(0U)
    {
        CASTLE_ASSERT(nodes <= N, // LCOV_EXCL_BR_LINE
                      CASTLE_ERROR_GENERIC("castle::dsa::basic_graph: node count exceeds capacity"));
        node_count_ = static_cast<index_type>(nodes <= N ? nodes : N); // LCOV_EXCL_BR_LINE
        clear();
    }

    /**
     * @brief Constructs a graph with `nodes` nodes from a list of edges.
     * @param nodes Number of nodes, `0 .. N`.
     * @param edges Edges added in order with `add_edge()`.
     * @note Complexity: O(N + M + sum of the visited adjacency lengths).
     * @warning An edge that `add_edge()` rejects reports through `CASTLE_ASSERT`; the edges before
     * it stay in the graph and the rest are skipped.
     */
    basic_graph(size_type nodes, initializer_list<edge_type> edges) CASTLE_NOEXCEPT : basic_graph(nodes)
    {
        CASTLE_CONST status result = add_edges(make_array_view(edges));
        CASTLE_ASSERT(result == status::ok, // LCOV_EXCL_BR_LINE
                      CASTLE_ERROR_GENERIC("castle::dsa::basic_graph: invalid edge in initializer list"));
        static_cast<void>(result);
    }

    /**
     * @brief Replaces the graph with `nodes` nodes and no edges.
     * @param nodes New number of nodes, `0 .. N`.
     * @return `status::ok`, or `status::out_of_range` when `nodes > N` (the graph is unchanged).
     * @note Complexity: O(N).
     */
    status assign(size_type nodes) CASTLE_NOEXCEPT
    {
        if (nodes > N)
        {
            return status::out_of_range;
        }
        node_count_ = static_cast<index_type>(nodes);
        clear();
        return status::ok;
    }

    /**
     * @brief Removes every edge and keeps `size()`.
     * @note Complexity: O(N).
     */
    void clear() CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < N; ++i)
        {
            head_[i] = no_arc;
        }
        free_slot_ = no_arc;
        used_slots_ = 0U;
        edge_count_ = 0U;
    }

    /** @brief Returns the number of nodes; valid node indices are `0 .. size() - 1`. */
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return node_count_; }

    /** @brief Returns the compile-time node capacity `N`. */
    CASTLE_CONSTEXPR size_type capacity() CASTLE_CONST CASTLE_NOEXCEPT { return N; }

    /** @brief Reports whether the graph has no nodes. */
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return node_count_ == 0U; }

    /** @brief Returns the number of edges (an undirected edge counts once). */
    size_type edge_count() CASTLE_CONST CASTLE_NOEXCEPT { return edge_count_; }

    /** @brief Returns the compile-time edge capacity `M`. */
    CASTLE_CONSTEXPR size_type edge_capacity() CASTLE_CONST CASTLE_NOEXCEPT { return M; }

    /** @brief Reports whether all `M` edge slots are in use. */
    bool full() CASTLE_CONST CASTLE_NOEXCEPT { return edge_count_ == M; }

    /**
     * @brief Adds the unweighted edge `from -> to` (or `from - to` for undirected graphs).
     * @param from Source node.
     * @param to Target node.
     * @return `status::ok`; `status::out_of_range` for an invalid node; `status::already_exists`
     * when the edge is present; `status::full` when `M` edges are stored.
     * @note Available for unweighted graphs only. Complexity: O(degree(from) + degree(to)).
     */
    template <bool IsWeighted = weighted, meta::enable_if_t<!IsWeighted, int> = 0>
    status add_edge(size_type from, size_type to) CASTLE_NOEXCEPT
    {
        return insert(from, to, 1U);
    }

    /**
     * @brief Adds the weighted edge `from -> to` (or `from - to` for undirected graphs).
     * @param from Source node.
     * @param to Target node.
     * @param weight Edge weight.
     * @return Same as the unweighted overload.
     * @note Available for weighted graphs only. Complexity: O(degree(from) + degree(to)).
     */
    template <bool IsWeighted = weighted, meta::enable_if_t<IsWeighted, int> = 0>
    status add_edge(size_type from, size_type to, weight_type weight) CASTLE_NOEXCEPT
    {
        return insert(from, to, weight);
    }

    /**
     * @brief Adds the edge described by `edge`.
     * @param edge `{from, to}` or `{from, to, weight}`.
     * @return Same as `add_edge(from, to[, weight])`.
     */
    status add_edge(CASTLE_CONST edge_type& edge) CASTLE_NOEXCEPT
    {
        CASTLE_IF_CONSTEXPR (weighted)
        {
            return insert(edge.from, edge.to, edge.weight);
        }
        else
        {
            return insert(edge.from, edge.to, 1U);
        }
    }

    /**
     * @brief Adds edges in order and stops at the first rejected one.
     * @param edges Edges to add.
     * @return `status::ok`, or the status of the first rejected edge (earlier edges stay added).
     */
    status add_edges(array_view<CASTLE_CONST edge_type> edges) CASTLE_NOEXCEPT
    {
        for (CASTLE_CONST edge_type& edge : edges)
        {
            CASTLE_CONST status result = add_edge(edge);
            if (result != status::ok)
            {
                return result;
            }
        }
        return status::ok;
    }

    /**
     * @brief Removes the edge `from -> to` (both arcs for undirected graphs).
     * @param from Source node.
     * @param to Target node.
     * @return `status::ok`; `status::out_of_range` for an invalid node; `status::not_found` when
     * the edge does not exist.
     * @note The freed slot is reused by the next `add_edge()`. Complexity: O(degree(from) + degree(to)).
     */
    status remove_edge(size_type from, size_type to) CASTLE_NOEXCEPT
    {
        if ((from >= node_count_) || (to >= node_count_))
        {
            return status::out_of_range;
        }
        CASTLE_CONST size_type arc = find_arc(from, to);
        if (arc == npos)
        {
            return status::not_found;
        }
        unlink(from, arc);
        CASTLE_IF_CONSTEXPR (!Directed)
        {
            unlink(to, arc ^ 1U);
        }
        release(edge_index(arc));
        return status::ok;
    }

    /**
     * @brief Reports whether the edge `from -> to` exists.
     * @return `false` also when a node is out of range. Complexity: O(degree(from)).
     */
    bool has_edge(size_type from, size_type to) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return find_arc(from, to) != npos;
    }

    /**
     * @brief Returns the number of outgoing arcs of `node` (a self-loop counts twice in undirected graphs).
     * @return Degree, or 0 when `node` is out of range. Complexity: O(degree(node)).
     */
    size_type degree(size_type node) CASTLE_CONST CASTLE_NOEXCEPT
    {
        size_type count = 0U;
        for (size_type arc = first_arc(node); arc != npos; arc = next_arc(arc))
        {
            ++count;
        }
        return count;
    }

    /**
     * @brief Returns the adjacency list of `node` in insertion order.
     * @return Range of `neighbor` entries; empty when `node` is out of range.
     */
    neighbor_range neighbors(size_type node) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return neighbor_range(this, first_arc(node));
    }

    /**
     * @brief Returns the arc handle of the edge `from -> to`.
     * @return Arc handle, or `npos` when the edge does not exist or `from` is out of range.
     * @note Complexity: O(degree(from)).
     */
    size_type find_arc(size_type from, size_type to) CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (size_type arc = first_arc(from); arc != npos; arc = next_arc(arc))
        {
            if (target_[arc] == to)
            {
                return arc;
            }
        }
        return npos;
    }

    /**
     * @brief Returns the first outgoing arc of `node`.
     * @return Arc handle, or `npos` when `node` has no arcs or is out of range.
     */
    size_type first_arc(size_type node) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (node >= node_count_)
        {
            return npos;
        }
        return handle(head_[node]);
    }

    /**
     * @brief Returns the arc after `arc` in the same adjacency list.
     * @param arc Live arc handle.
     * @return Arc handle, or `npos` at the end of the list.
     */
    size_type next_arc(size_type arc) CASTLE_CONST CASTLE_NOEXCEPT { return handle(next_[arc]); }

    /**
     * @brief Returns the node an arc points to.
     * @param arc Live arc handle.
     */
    size_type target(size_type arc) CASTLE_CONST CASTLE_NOEXCEPT { return target_[arc]; }

    /**
     * @brief Returns the weight of an arc (1 for unweighted graphs).
     * @param arc Live arc handle.
     */
    weight_type weight(size_type arc) CASTLE_CONST CASTLE_NOEXCEPT { return this->stored_weight(arc); }

    /**
     * @brief Returns the edge slot an arc belongs to.
     * @param arc Live arc handle.
     * @return Slot index in `0 .. M - 1`. Both arcs of an undirected edge share the same slot.
     */
    size_type edge_index(size_type arc) CASTLE_CONST CASTLE_NOEXCEPT { return Directed ? arc : arc / 2U; }

private:
    /** @brief Marker for "no arc" / "no free slot". */
    static CASTLE_CONSTEXPR arc_index_type no_arc = static_cast<arc_index_type>(static_arc_capacity);

    /** @brief Converts a stored arc index into a public handle. */
    static size_type handle(arc_index_type arc) CASTLE_NOEXCEPT
    {
        return arc == no_arc ? npos : static_cast<size_type>(arc);
    }

    /** @brief Returns the first arc of an edge slot; an undirected slot also owns the next arc. */
    static size_type slot_arc(size_type slot) CASTLE_NOEXCEPT { return Directed ? slot : slot * 2U; }

    /**
     * @brief Validates and stores a new edge.
     * @param from Source node.
     * @param to Target node.
     * @param weight Weight stored on every arc of the edge.
     */
    status insert(size_type from, size_type to, weight_type weight) CASTLE_NOEXCEPT
    {
        if ((from >= node_count_) || (to >= node_count_))
        {
            return status::out_of_range;
        }
        // One walk both rejects duplicates and finds the tail that keeps insertion order.
        arc_index_type tail = no_arc;
        for (arc_index_type arc = head_[from]; arc != no_arc; arc = next_[arc])
        {
            if (target_[arc] == to)
            {
                return status::already_exists;
            }
            tail = arc;
        }
        if (edge_count_ == M)
        {
            return status::full;
        }

        CASTLE_CONST size_type arc = slot_arc(acquire());
        link(from, tail, arc, to, weight);
        CASTLE_IF_CONSTEXPR (!Directed)
        {
            link(to, last_arc(to), arc + 1U, from, weight);
        }
        ++edge_count_;
        return status::ok;
    }

    /** @brief Returns the last arc of `node`, or `no_arc` for an empty list. */
    arc_index_type last_arc(size_type node) CASTLE_CONST CASTLE_NOEXCEPT
    {
        arc_index_type tail = no_arc;
        for (arc_index_type arc = head_[node]; arc != no_arc; arc = next_[arc])
        {
            tail = arc;
        }
        return tail;
    }

    /** @brief Fills `arc` and appends it after `tail` in the list of `node`. */
    void link(size_type node, arc_index_type tail, size_type arc, size_type to, weight_type weight) CASTLE_NOEXCEPT
    {
        target_[arc] = static_cast<index_type>(to);
        this->store_weight(arc, weight);
        next_[arc] = no_arc;
        if (tail == no_arc)
        {
            head_[node] = static_cast<arc_index_type>(arc);
        }
        else
        {
            next_[tail] = static_cast<arc_index_type>(arc);
        }
    }

    /** @brief Removes `arc`, which must be in the list of `node`, from that list. */
    void unlink(size_type node, size_type arc) CASTLE_NOEXCEPT
    {
        arc_index_type previous = no_arc;
        arc_index_type current = head_[node];
        while (current != arc)
        {
            previous = current;
            current = next_[current];
        }
        if (previous == no_arc)
        {
            head_[node] = next_[current];
        }
        else
        {
            next_[previous] = next_[current];
        }
    }

    /** @brief Takes a free edge slot: recycled slots first, then never-used ones. */
    size_type acquire() CASTLE_NOEXCEPT
    {
        if (free_slot_ != no_arc)
        {
            CASTLE_CONST size_type slot = free_slot_;
            free_slot_ = next_[slot_arc(slot)];
            return slot;
        }
        return used_slots_++;
    }

    /** @brief Returns an edge slot to the free list. */
    void release(size_type slot) CASTLE_NOEXCEPT
    {
        next_[slot_arc(slot)] = free_slot_;
        free_slot_ = static_cast<arc_index_type>(slot);
        --edge_count_;
    }

    arc_index_type head_[N];
    arc_index_type next_[static_arc_capacity];
    index_type target_[static_arc_capacity];
    arc_index_type free_slot_;
    arc_index_type used_slots_;
    arc_index_type edge_count_;
    index_type node_count_;
};

/**
 * @brief Undirected, unweighted graph with up to `N` nodes and `M` edges.
 */
template <size_type N, size_type M>
using undirected_graph = basic_graph<N, M, false, void>;

/**
 * @brief Directed, unweighted graph with up to `N` nodes and `M` edges.
 */
template <size_type N, size_type M>
using directed_graph = basic_graph<N, M, true, void>;

/**
 * @brief Undirected graph with up to `N` nodes and `M` edges of weight type `W`.
 */
template <size_type N, size_type M, typename W = int32_t>
using undirected_weighted_graph = basic_graph<N, M, false, W>;

/**
 * @brief Directed graph with up to `N` nodes and `M` edges of weight type `W`.
 */
template <size_type N, size_type M, typename W = int32_t>
using directed_weighted_graph = basic_graph<N, M, true, W>;

/**
 * @brief Algorithms over `basic_graph` instances.
 * @note Every function works on nodes `0 .. graph.size() - 1`. Scratch storage is a set of local
 * arrays sized by the graph's capacities (documented per function); nothing is allocated and
 * nothing recurses.
 */
namespace graph_algorithms
{

/** @brief Sequence of nodes (traversal order, path, topological order, articulation points). */
template <typename Graph>
using node_list = vector<typename Graph::index_type, Graph::static_capacity>;

/** @brief Color per node: `colors[v]` is the color of node `v`. */
template <typename Graph>
using color_map = node_list<Graph>;

/** @brief List of edges (spanning tree, bridges). */
template <typename Graph>
using edge_list = vector<typename Graph::edge_type, Graph::static_capacity>;

/** @brief All-pairs distance matrix: `matrix(from, to)`. */
template <typename Graph>
using distance_matrix = grid<typename Graph::weight_type, Graph::static_capacity, Graph::static_capacity>;

/** @brief Partition of the nodes into components. */
template <typename Graph>
using component_set = disjoint_set<Graph::static_capacity>;

/** @brief Distance reported for unreachable nodes: the largest `weight_type` value. */
template <typename Graph>
CASTLE_CONSTEXPR typename Graph::weight_type infinity = numeric_limits<typename Graph::weight_type>::max();

namespace detail
{
template <typename Graph>
struct graph_search;
} // namespace detail

/**
 * @brief Single-source shortest-path result: distance and predecessor of every node.
 * @tparam Graph Graph type the tree was computed on.
 * @note Filled by `shortest_paths()` and `bellman_ford()`. A failed run leaves the tree empty
 * (`size() == 0`), so every query returns its neutral value.
 */
template <typename Graph>
class shortest_path_tree
{
public:
    using size_type = castle::size_type;
    using index_type = typename Graph::index_type;
    using weight_type = typename Graph::weight_type;

    /** @brief Value returned by node queries without an answer. */
    static CASTLE_CONSTEXPR size_type npos = Graph::npos;

    /** @brief Constructs an empty tree. */
    shortest_path_tree() CASTLE_NOEXCEPT : distance_{}, parent_{}, node_count_(0U), source_(0U) {}

    /** @brief Returns the number of nodes covered; 0 before a successful run. */
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return node_count_; }

    /** @brief Reports whether the tree holds no result. */
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return node_count_ == 0U; }

    /** @brief Returns the source node, or `npos` for an empty tree. */
    size_type source() CASTLE_CONST CASTLE_NOEXCEPT { return node_count_ == 0U ? npos : source_; }

    /** @brief Reports whether `node` is reachable from the source. */
    bool reachable(size_type node) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (node < node_count_) && (distance_[node] != infinity<Graph>);
    }

    /**
     * @brief Returns the shortest distance from the source to `node`.
     * @return Distance, or `infinity<Graph>` when unreachable or out of range.
     */
    weight_type distance(size_type node) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return node < node_count_ ? distance_[node] : infinity<Graph>;
    }

    /**
     * @brief Returns the predecessor of `node` on its shortest path.
     * @return Parent node, or `npos` for the source, unreachable, or out-of-range nodes.
     */
    size_type parent(size_type node) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if ((node >= node_count_) || (parent_[node] == no_parent))
        {
            return npos;
        }
        return parent_[node];
    }

    /**
     * @brief Writes the shortest path from the source to `node`, both ends included.
     * @param node Destination node.
     * @param path Cleared, then filled in source-to-`node` order.
     * @return `status::ok`; `status::out_of_range` for an invalid node; `status::not_found` when
     * `node` is unreachable.
     * @note Complexity: O(path length).
     */
    status path_to(size_type node, node_list<Graph>& path) CASTLE_CONST CASTLE_NOEXCEPT
    {
        path.clear();
        if (node >= node_count_)
        {
            return status::out_of_range;
        }
        if (distance_[node] == infinity<Graph>)
        {
            return status::not_found;
        }
        for (size_type current = node; current != no_parent; current = parent_[current])
        {
            path.push_back(static_cast<index_type>(current));
        }
        for (size_type low = 0U, high = path.size() - 1U; low < high; ++low, --high)
        {
            castle::swap(path[low], path[high]);
        }
        return status::ok;
    }

private:
    friend struct detail::graph_search<Graph>;

    /** @brief Parent marker of the source and of unreached nodes. */
    static CASTLE_CONSTEXPR index_type no_parent = static_cast<index_type>(Graph::static_capacity);

    /** @brief Starts a run over `nodes` nodes: every distance infinite, no parents. */
    void reset(size_type nodes, size_type source) CASTLE_NOEXCEPT
    {
        node_count_ = static_cast<index_type>(nodes);
        source_ = static_cast<index_type>(source);
        for (size_type i = 0U; i < nodes; ++i)
        {
            distance_[i] = infinity<Graph>;
            parent_[i] = no_parent;
        }
    }

    weight_type distance_[Graph::static_capacity];
    index_type parent_[Graph::static_capacity];
    index_type node_count_;
    index_type source_;
};

namespace detail
{

/** @brief One bit per node. */
template <typename Graph>
using node_mask = bitset<Graph::static_capacity>;

/**
 * @brief Calls a traversal visitor.
 * @return The visitor's result when it returns `bool`, otherwise `true` (keep going).
 */
template <typename Visitor>
bool invoke_visitor(Visitor& visitor, size_type node) CASTLE_NOEXCEPT
{
    CASTLE_IF_CONSTEXPR (meta::is_void<decltype(visitor(node))>::value)
    {
        visitor(node);
        return true;
    }
    else
    {
        return static_cast<bool>(visitor(node));
    }
}

/** @brief Packs an arc handle into the graph's compact arc index type. */
template <typename Graph>
typename Graph::arc_index_type pack_arc(size_type arc) CASTLE_NOEXCEPT
{
    return static_cast<typename Graph::arc_index_type>(arc == Graph::npos ? Graph::static_arc_capacity : arc);
}

/** @brief Unpacks a compact arc index into an arc handle. */
template <typename Graph>
size_type unpack_arc(typename Graph::arc_index_type arc) CASTLE_NOEXCEPT
{
    return arc == Graph::static_arc_capacity ? Graph::npos : static_cast<size_type>(arc);
}

/** @brief Builds the reported edge for the arc `from -> graph.target(arc)`. */
template <typename Graph>
typename Graph::edge_type make_edge(CASTLE_CONST Graph& graph, size_type from, size_type arc) CASTLE_NOEXCEPT
{
    typename Graph::edge_type edge{};
    edge.from = from;
    edge.to = graph.target(arc);
    CASTLE_IF_CONSTEXPR (Graph::weighted)
    {
        edge.weight = graph.weight(arc);
    }
    return edge;
}

/**
 * @brief Iterative depth-first walk from `root` with event hooks.
 * @param graph Graph to walk.
 * @param root Start node, not yet in `visited`.
 * @param visited Nodes already discovered; updated by the walk.
 * @param hooks Object providing:
 *   - `bool enter(node, entry_arc)` when a node is discovered (`entry_arc == npos` for `root`);
 *     returning `false` aborts the walk;
 *   - `void revisit(node, next, arc, entry_arc)` for an arc to an already discovered node;
 *   - `void leave(node, parent, entry_arc)` once all arcs of `node` are done (`parent == npos` for `root`).
 * @note Frames keep an arc cursor, so the visit order matches the recursive formulation.
 * Stack: N node indices and 2N arc indices.
 */
template <typename Graph, typename Hooks>
void dfs_walk(CASTLE_CONST Graph& graph, size_type root, node_mask<Graph>& visited, Hooks& hooks) CASTLE_NOEXCEPT
{
    using index_type = typename Graph::index_type;
    using arc_index_type = typename Graph::arc_index_type;

    index_type nodes[Graph::static_capacity];
    arc_index_type cursor[Graph::static_capacity];
    arc_index_type entry[Graph::static_capacity];

    visited.set(root);
    if (!hooks.enter(root, Graph::npos))
    {
        return;
    }
    nodes[0U] = static_cast<index_type>(root);
    cursor[0U] = pack_arc<Graph>(graph.first_arc(root));
    entry[0U] = pack_arc<Graph>(Graph::npos);
    size_type depth = 1U;

    while (depth > 0U)
    {
        CASTLE_CONST size_type top = depth - 1U;
        CASTLE_CONST size_type node = nodes[top];
        CASTLE_CONST size_type arc = unpack_arc<Graph>(cursor[top]);
        if (arc == Graph::npos)
        {
            depth = top;
            hooks.leave(node, depth > 0U ? static_cast<size_type>(nodes[depth - 1U]) : Graph::npos,
                        unpack_arc<Graph>(entry[top]));
            continue;
        }
        cursor[top] = pack_arc<Graph>(graph.next_arc(arc));

        CASTLE_CONST size_type next = graph.target(arc);
        if (visited.test(next))
        {
            hooks.revisit(node, next, arc, unpack_arc<Graph>(entry[top]));
            continue;
        }
        visited.set(next);
        if (!hooks.enter(next, arc))
        {
            return;
        }
        nodes[depth] = static_cast<index_type>(next);
        cursor[depth] = pack_arc<Graph>(graph.first_arc(next));
        entry[depth] = pack_arc<Graph>(arc);
        ++depth;
    }
}

/** @brief DFS hooks forwarding discoveries to a user visitor. */
template <typename Visitor>
struct visitor_hooks
{
    Visitor& visitor;
    size_type count;

    bool enter(size_type node, size_type) CASTLE_NOEXCEPT
    {
        ++count;
        return invoke_visitor(visitor, node);
    }

    void revisit(size_type, size_type, size_type, size_type) CASTLE_NOEXCEPT {}

    void leave(size_type, size_type, size_type) CASTLE_NOEXCEPT {}
};

/** @brief DFS hooks of Tarjan's strongly connected components algorithm. */
template <typename Graph>
struct tarjan_hooks
{
    using index_type = typename Graph::index_type;

    explicit tarjan_hooks(component_set<Graph>& components) CASTLE_NOEXCEPT
        : order{}, low{}, stack{}, stack_size(0U), timer(0U), on_stack(), sets(components)
    {
    }

    bool enter(size_type node, size_type) CASTLE_NOEXCEPT
    {
        order[node] = static_cast<index_type>(timer);
        low[node] = order[node];
        ++timer;
        stack[stack_size] = static_cast<index_type>(node);
        ++stack_size;
        on_stack.set(node);
        return true;
    }

    void revisit(size_type node, size_type next, size_type, size_type) CASTLE_NOEXCEPT
    {
        if (on_stack.test(next) && (order[next] < low[node]))
        {
            low[node] = order[next];
        }
    }

    void leave(size_type node, size_type parent, size_type) CASTLE_NOEXCEPT
    {
        if (low[node] == order[node])
        {
            // `node` roots a component: everything above it on the stack belongs to it.
            size_type member = Graph::npos;
            while (member != node)
            {
                --stack_size;
                member = stack[stack_size];
                on_stack.reset(member);
                sets.unite(node, member);
            }
        }
        if ((parent != Graph::npos) && (low[node] < low[parent]))
        {
            low[parent] = low[node];
        }
    }

    index_type order[Graph::static_capacity];
    index_type low[Graph::static_capacity];
    index_type stack[Graph::static_capacity];
    size_type stack_size;
    size_type timer;
    node_mask<Graph> on_stack;
    component_set<Graph>& sets;
};

/** @brief DFS hooks of Tarjan's low-link algorithm for bridges and articulation points. */
template <typename Graph>
struct low_link_hooks
{
    using index_type = typename Graph::index_type;

    low_link_hooks(CASTLE_CONST Graph& g, edge_list<Graph>* bridge_out, node_mask<Graph>* cut_out) CASTLE_NOEXCEPT
        : graph(g), order{}, low{}, timer(0U), root(0U), root_children(0U), bridges(bridge_out), cut_nodes(cut_out)
    {
    }

    bool enter(size_type node, size_type) CASTLE_NOEXCEPT
    {
        order[node] = static_cast<index_type>(timer);
        low[node] = order[node];
        ++timer;
        return true;
    }

    void revisit(size_type node, size_type next, size_type arc, size_type entry_arc) CASTLE_NOEXCEPT
    {
        // The twin of the entry arc leads back to the parent through the same edge: not a back edge.
        if ((entry_arc != Graph::npos) && (graph.edge_index(arc) == graph.edge_index(entry_arc)))
        {
            return;
        }
        if (order[next] < low[node])
        {
            low[node] = order[next];
        }
    }

    void leave(size_type node, size_type parent, size_type entry_arc) CASTLE_NOEXCEPT
    {
        if (parent == Graph::npos)
        {
            return;
        }
        if (low[node] < low[parent])
        {
            low[parent] = low[node];
        }
        if ((bridges != nullptr) && (low[node] > order[parent]))
        {
            bridges->push_back(make_edge(graph, parent, entry_arc));
        }
        if (parent == root)
        {
            ++root_children;
        }
        else if ((cut_nodes != nullptr) && (low[node] >= order[parent]))
        {
            cut_nodes->set(parent);
        }
    }

    CASTLE_CONST Graph& graph;
    index_type order[Graph::static_capacity];
    index_type low[Graph::static_capacity];
    size_type timer;
    size_type root;
    size_type root_children;
    edge_list<Graph>* bridges;
    node_mask<Graph>* cut_nodes;
};

/** @brief Runs the low-link walk from every undiscovered node. */
template <typename Graph>
void low_link(CASTLE_CONST Graph& graph, edge_list<Graph>* bridges, node_mask<Graph>* cut_nodes) CASTLE_NOEXCEPT
{
    static_assert(!Graph::directed, "graph_algorithms: bridges and articulation points require an undirected graph");
    low_link_hooks<Graph> hooks(graph, bridges, cut_nodes);
    node_mask<Graph> visited;
    for (size_type root = 0U; root < graph.size(); ++root)
    {
        if (visited.test(root))
        {
            continue;
        }
        hooks.root = root;
        hooks.root_children = 0U;
        dfs_walk(graph, root, visited, hooks);
        if ((cut_nodes != nullptr) && (hooks.root_children > 1U))
        {
            cut_nodes->set(root);
        }
    }
}

/** @brief Single-source shortest-path engines writing into a `shortest_path_tree`. */
template <typename Graph>
struct graph_search
{
    using index_type = typename Graph::index_type;
    using weight_type = typename Graph::weight_type;
    using tree_type = shortest_path_tree<Graph>;

    /**
     * @brief Indexed binary min-heap of nodes keyed by the tree distances, with decrease-key.
     * @note Stores 2N node indices.
     */
    class min_queue
    {
    public:
        min_queue(CASTLE_CONST weight_type* key, size_type nodes) CASTLE_NOEXCEPT
            : key_(key), heap_{}, position_{}, size_(0U)
        {
            for (size_type i = 0U; i < nodes; ++i)
            {
                position_[i] = absent;
            }
        }

        bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return size_ == 0U; }

        /** @brief Inserts `node`, or restores order after its key decreased. */
        void push_or_decrease(size_type node) CASTLE_NOEXCEPT
        {
            size_type slot = position_[node];
            if (slot == absent)
            {
                slot = size_;
                ++size_;
                place(slot, node);
            }
            while (slot > 0U)
            {
                CASTLE_CONST size_type parent = (slot - 1U) / 2U;
                if (!(key_[heap_[slot]] < key_[heap_[parent]]))
                {
                    break;
                }
                exchange(slot, parent);
                slot = parent;
            }
        }

        /** @brief Removes and returns the node with the smallest key. */
        size_type pop() CASTLE_NOEXCEPT
        {
            CASTLE_CONST size_type top = heap_[0U];
            position_[top] = absent;
            --size_;
            if (size_ == 0U)
            {
                return top;
            }
            place(0U, heap_[size_]);
            size_type slot = 0U;
            for (;;)
            {
                CASTLE_CONST size_type left = slot * 2U + 1U;
                if (left >= size_)
                {
                    break;
                }
                size_type child = left;
                if ((left + 1U < size_) && (key_[heap_[left + 1U]] < key_[heap_[left]]))
                {
                    child = left + 1U;
                }
                if (!(key_[heap_[child]] < key_[heap_[slot]]))
                {
                    break;
                }
                exchange(slot, child);
                slot = child;
            }
            return top;
        }

    private:
        static CASTLE_CONSTEXPR index_type absent = static_cast<index_type>(Graph::static_capacity);

        void place(size_type slot, size_type node) CASTLE_NOEXCEPT
        {
            heap_[slot] = static_cast<index_type>(node);
            position_[node] = static_cast<index_type>(slot);
        }

        void exchange(size_type lhs, size_type rhs) CASTLE_NOEXCEPT
        {
            CASTLE_CONST size_type node = heap_[lhs];
            place(lhs, heap_[rhs]);
            place(rhs, node);
        }

        CASTLE_CONST weight_type* key_;
        index_type heap_[Graph::static_capacity];
        index_type position_[Graph::static_capacity];
        size_type size_;
    };

    /**
     * @brief BFS for unweighted graphs, Dijkstra for weighted graphs.
     * @param target Node whose settlement stops the search early, or `npos` for a full run.
     */
    static status run(CASTLE_CONST Graph& graph, size_type source, size_type target, tree_type& tree) CASTLE_NOEXCEPT
    {
        if (source >= graph.size())
        {
            tree.reset(0U, 0U);
            return status::out_of_range;
        }
        tree.reset(graph.size(), source);
        tree.distance_[source] = static_cast<weight_type>(0);
        CASTLE_IF_CONSTEXPR (Graph::weighted)
        {
            return dijkstra(graph, target, tree);
        }
        else
        {
            breadth_first(graph, target, tree);
            return status::ok;
        }
    }

    /** @brief Hop distances by BFS. Stack: N node indices. */
    static void breadth_first(CASTLE_CONST Graph& graph, size_type target, tree_type& tree) CASTLE_NOEXCEPT
    {
        index_type queue[Graph::static_capacity];
        size_type head = 0U;
        size_type tail = 1U;
        queue[0U] = tree.source_;
        while (head < tail)
        {
            CASTLE_CONST size_type node = queue[head];
            ++head;
            if (node == target)
            {
                return;
            }
            for (size_type arc = graph.first_arc(node); arc != Graph::npos; arc = graph.next_arc(arc))
            {
                CASTLE_CONST size_type next = graph.target(arc);
                if (tree.distance_[next] == infinity<Graph>)
                {
                    tree.distance_[next] = tree.distance_[node] + 1U;
                    tree.parent_[next] = static_cast<index_type>(node);
                    queue[tail] = static_cast<index_type>(next);
                    ++tail;
                }
            }
        }
    }

    /** @brief Dijkstra with an indexed heap. Rejects negative arcs it scans. */
    static status dijkstra(CASTLE_CONST Graph& graph, size_type target, tree_type& tree) CASTLE_NOEXCEPT
    {
        min_queue queue(tree.distance_, graph.size());
        queue.push_or_decrease(tree.source_);
        while (!queue.empty())
        {
            CASTLE_CONST size_type node = queue.pop();
            if (node == target)
            {
                break;
            }
            for (size_type arc = graph.first_arc(node); arc != Graph::npos; arc = graph.next_arc(arc))
            {
                CASTLE_CONST weight_type weight = graph.weight(arc);
                CASTLE_IF_CONSTEXPR (meta::is_signed<weight_type>::value)
                {
                    if (weight < static_cast<weight_type>(0))
                    {
                        tree.reset(0U, 0U);
                        return status::invalid_argument;
                    }
                }
                if (relax(tree, node, graph.target(arc), weight))
                {
                    queue.push_or_decrease(graph.target(arc));
                }
            }
        }
        return status::ok;
    }

    /** @brief Bellman-Ford with early exit; detects negative cycles reachable from the source. */
    static status bellman_ford(CASTLE_CONST Graph& graph, size_type source, tree_type& tree) CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type nodes = graph.size();
        if (source >= nodes)
        {
            tree.reset(0U, 0U);
            return status::out_of_range;
        }
        tree.reset(nodes, source);
        tree.distance_[source] = static_cast<weight_type>(0);
        // Without a negative cycle every distance is final after nodes - 1 rounds, so a change
        // in round `nodes` proves one.
        for (size_type round = 0U; round < nodes; ++round)
        {
            bool changed = false;
            for (size_type node = 0U; node < nodes; ++node)
            {
                if (tree.distance_[node] == infinity<Graph>)
                {
                    continue;
                }
                for (size_type arc = graph.first_arc(node); arc != Graph::npos; arc = graph.next_arc(arc))
                {
                    if (relax(tree, node, graph.target(arc), graph.weight(arc)))
                    {
                        changed = true;
                    }
                }
            }
            if (!changed)
            {
                return status::ok;
            }
        }
        tree.reset(0U, 0U);
        return status::invalid_argument;
    }

    /** @brief Lowers the distance of `next` through `node` when shorter. */
    static bool relax(tree_type& tree, size_type node, size_type next, weight_type weight) CASTLE_NOEXCEPT
    {
        CASTLE_CONST weight_type candidate = static_cast<weight_type>(tree.distance_[node] + weight);
        if (!(candidate < tree.distance_[next]))
        {
            return false;
        }
        tree.distance_[next] = candidate;
        tree.parent_[next] = static_cast<index_type>(node);
        return true;
    }
};

} // namespace detail

/**
 * @brief Breadth-first traversal from `source`.
 * @param graph Graph to traverse.
 * @param source Start node.
 * @param visitor Callable `visitor(size_type node)` invoked once per reached node in BFS order.
 * It may return `void`, or `bool` where `false` stops the traversal.
 * @return Number of visitor calls; 0 when `source` is out of range.
 * @note Complexity: O(V + E). Stack: N node indices plus an N-bit mask.
 */
template <typename Graph, typename Visitor>
size_type bfs(CASTLE_CONST Graph& graph, size_type source, Visitor visitor) CASTLE_NOEXCEPT
{
    if (source >= graph.size())
    {
        return 0U;
    }
    typename Graph::index_type queue[Graph::static_capacity];
    detail::node_mask<Graph> visited;
    visited.set(source);
    queue[0U] = static_cast<typename Graph::index_type>(source);
    size_type head = 0U;
    size_type tail = 1U;
    while (head < tail)
    {
        CASTLE_CONST size_type node = queue[head];
        ++head;
        if (!detail::invoke_visitor(visitor, node))
        {
            break;
        }
        for (size_type arc = graph.first_arc(node); arc != Graph::npos; arc = graph.next_arc(arc))
        {
            CASTLE_CONST size_type next = graph.target(arc);
            if (!visited.test(next))
            {
                visited.set(next);
                queue[tail] = static_cast<typename Graph::index_type>(next);
                ++tail;
            }
        }
    }
    return head;
}

/**
 * @brief Depth-first (pre-order) traversal from `source`, iterative.
 * @param graph Graph to traverse.
 * @param source Start node.
 * @param visitor Same contract as for `bfs()`.
 * @return Number of visitor calls; 0 when `source` is out of range.
 * @note Neighbors are explored in insertion order, matching the recursive formulation.
 * Complexity: O(V + E). Stack: N node indices, 2N arc indices, an N-bit mask.
 */
template <typename Graph, typename Visitor>
size_type dfs(CASTLE_CONST Graph& graph, size_type source, Visitor visitor) CASTLE_NOEXCEPT
{
    if (source >= graph.size())
    {
        return 0U;
    }
    detail::visitor_hooks<Visitor> hooks{visitor, 0U};
    detail::node_mask<Graph> visited;
    detail::dfs_walk(graph, source, visited, hooks);
    return hooks.count;
}

/**
 * @brief Reports whether a path leads from `from` to `to`.
 * @return `true` also for `from == to`; `false` when a node is out of range.
 * @note BFS that stops at `to`. Complexity: O(V + E).
 */
template <typename Graph>
bool reachable(CASTLE_CONST Graph& graph, size_type from, size_type to) CASTLE_NOEXCEPT
{
    bool found = false;
    bfs(graph, from, [&found, to](size_type node) {
        found = (node == to);
        return !found;
    });
    return found;
}

/**
 * @brief Reports whether two nodes are connected.
 * @return Undirected graphs: same component. Directed graphs: strongly connected, i.e. each node
 * reaches the other.
 * @note Complexity: O(V + E).
 */
template <typename Graph>
bool connected(CASTLE_CONST Graph& graph, size_type lhs, size_type rhs) CASTLE_NOEXCEPT
{
    return reachable(graph, lhs, rhs) && (!Graph::directed || reachable(graph, rhs, lhs));
}

/**
 * @brief Partitions the nodes into connected components with union-find.
 * @return Components of an undirected graph; weakly connected components (direction ignored) of
 * a directed graph.
 * @note Complexity: O(V + E alpha(V)).
 */
template <typename Graph>
component_set<Graph> connected_components(CASTLE_CONST Graph& graph) CASTLE_NOEXCEPT
{
    component_set<Graph> components(graph.size());
    for (size_type node = 0U; node < graph.size(); ++node)
    {
        for (size_type arc = graph.first_arc(node); arc != Graph::npos; arc = graph.next_arc(arc))
        {
            components.unite(node, graph.target(arc));
        }
    }
    return components;
}

/**
 * @brief Partitions a directed graph into strongly connected components (Tarjan, iterative).
 * @note Complexity: O(V + E). Stack: about 6N indices plus two N-bit masks.
 */
template <typename Graph>
component_set<Graph> strongly_connected_components(CASTLE_CONST Graph& graph) CASTLE_NOEXCEPT
{
    static_assert(Graph::directed, "graph_algorithms::strongly_connected_components requires a directed graph");
    component_set<Graph> components(graph.size());
    detail::tarjan_hooks<Graph> hooks(components);
    detail::node_mask<Graph> visited;
    for (size_type root = 0U; root < graph.size(); ++root)
    {
        if (!visited.test(root))
        {
            detail::dfs_walk(graph, root, visited, hooks);
        }
    }
    return components;
}

/**
 * @brief Orders a directed graph so that every edge points forward (Kahn's algorithm, iterative).
 * @param graph Directed graph.
 * @param order Cleared, then filled with the order. Sources are released in ascending node order,
 * later nodes in the order their last incoming edge is consumed.
 * @return `status::ok`, or `status::invalid_argument` when the graph has a cycle (then `order`
 * holds only the nodes that do not depend on any cycle).
 * @note Complexity: O(V + E). Stack: N node indices.
 */
template <typename Graph>
status topological_sort(CASTLE_CONST Graph& graph, node_list<Graph>& order) CASTLE_NOEXCEPT
{
    static_assert(Graph::directed, "graph_algorithms::topological_sort requires a directed graph");
    using index_type = typename Graph::index_type;
    CASTLE_CONST size_type nodes = graph.size();

    index_type in_degree[Graph::static_capacity];
    for (size_type node = 0U; node < nodes; ++node)
    {
        in_degree[node] = 0U;
    }
    for (size_type node = 0U; node < nodes; ++node)
    {
        for (size_type arc = graph.first_arc(node); arc != Graph::npos; arc = graph.next_arc(arc))
        {
            ++in_degree[graph.target(arc)];
        }
    }

    // `order` doubles as the FIFO queue of released nodes.
    order.clear();
    for (size_type node = 0U; node < nodes; ++node)
    {
        if (in_degree[node] == 0U)
        {
            order.push_back(static_cast<index_type>(node));
        }
    }
    for (size_type i = 0U; i < order.size(); ++i)
    {
        for (size_type arc = graph.first_arc(order[i]); arc != Graph::npos; arc = graph.next_arc(arc))
        {
            CASTLE_CONST size_type next = graph.target(arc);
            --in_degree[next];
            if (in_degree[next] == 0U)
            {
                order.push_back(static_cast<index_type>(next));
            }
        }
    }
    return order.size() == nodes ? status::ok : status::invalid_argument;
}

/**
 * @brief Reports whether the graph contains a cycle.
 * @note Undirected: union-find over the edges. Directed: Kahn's algorithm. A self-loop is a cycle
 * in both. Complexity: O(V + E).
 */
template <typename Graph>
bool has_cycle(CASTLE_CONST Graph& graph) CASTLE_NOEXCEPT
{
    CASTLE_IF_CONSTEXPR (Graph::directed)
    {
        node_list<Graph> order;
        return topological_sort(graph, order) != status::ok;
    }
    else
    {
        component_set<Graph> components(graph.size());
        for (size_type node = 0U; node < graph.size(); ++node)
        {
            for (size_type arc = graph.first_arc(node); arc != Graph::npos; arc = graph.next_arc(arc))
            {
                // Each edge is seen from both ends; take it once, from its lower end.
                CASTLE_CONST size_type next = graph.target(arc);
                if ((next >= node) && !components.unite(node, next))
                {
                    return true;
                }
            }
        }
        return false;
    }
}

/**
 * @brief Single-source shortest paths: BFS on unweighted graphs, Dijkstra on weighted graphs.
 * @param graph Graph to search.
 * @param source Start node.
 * @param tree Receives distances and predecessors.
 * @return `status::ok`; `status::out_of_range` for an invalid source; `status::invalid_argument`
 * when a negative weight is met (use `bellman_ford()` instead). On failure `tree` is empty.
 * @note Complexity: O(V + E) unweighted, O((V + E) log V) weighted. Stack: 2N node indices.
 */
template <typename Graph>
status shortest_paths(CASTLE_CONST Graph& graph, size_type source, shortest_path_tree<Graph>& tree) CASTLE_NOEXCEPT
{
    return detail::graph_search<Graph>::run(graph, source, Graph::npos, tree);
}

/**
 * @brief Single-source shortest paths that allow negative weights (Bellman-Ford).
 * @return `status::ok`; `status::out_of_range` for an invalid source; `status::invalid_argument`
 * when a negative cycle is reachable from `source` (an undirected negative edge is such a cycle).
 * On failure `tree` is empty.
 * @note Complexity: O(V E), early exit once a round changes nothing. No scratch storage.
 */
template <typename Graph>
status bellman_ford(CASTLE_CONST Graph& graph, size_type source, shortest_path_tree<Graph>& tree) CASTLE_NOEXCEPT
{
    return detail::graph_search<Graph>::bellman_ford(graph, source, tree);
}

/**
 * @brief All-pairs shortest distances (Floyd-Warshall).
 * @param graph Graph to search.
 * @param distances Receives `distances(from, to)` for `from, to < graph.size()`; unreachable pairs
 * hold `infinity<Graph>`. Entries outside that block are not touched.
 * @return `status::ok`, or `status::invalid_argument` when a negative cycle exists (some
 * `distances(v, v) < 0`; the matrix is then not meaningful).
 * @note Complexity: O(V^3). The matrix holds N * N weights; place it where that fits.
 */
template <typename Graph>
status floyd_warshall(CASTLE_CONST Graph& graph, distance_matrix<Graph>& distances) CASTLE_NOEXCEPT
{
    using weight_type = typename Graph::weight_type;
    CASTLE_CONST size_type nodes = graph.size();
    for (size_type from = 0U; from < nodes; ++from)
    {
        for (size_type to = 0U; to < nodes; ++to)
        {
            distances(from, to) = (from == to) ? static_cast<weight_type>(0) : infinity<Graph>;
        }
        for (size_type arc = graph.first_arc(from); arc != Graph::npos; arc = graph.next_arc(arc))
        {
            weight_type& cell = distances(from, graph.target(arc));
            if (graph.weight(arc) < cell)
            {
                cell = graph.weight(arc);
            }
        }
    }
    for (size_type via = 0U; via < nodes; ++via)
    {
        for (size_type from = 0U; from < nodes; ++from)
        {
            CASTLE_CONST weight_type first_leg = distances(from, via);
            if (first_leg == infinity<Graph>)
            {
                continue;
            }
            for (size_type to = 0U; to < nodes; ++to)
            {
                CASTLE_CONST weight_type second_leg = distances(via, to);
                if (second_leg == infinity<Graph>)
                {
                    continue;
                }
                CASTLE_CONST weight_type candidate = static_cast<weight_type>(first_leg + second_leg);
                if (candidate < distances(from, to))
                {
                    distances(from, to) = candidate;
                }
            }
        }
    }
    CASTLE_IF_CONSTEXPR (meta::is_signed<weight_type>::value)
    {
        for (size_type node = 0U; node < nodes; ++node)
        {
            if (distances(node, node) < static_cast<weight_type>(0))
            {
                return status::invalid_argument;
            }
        }
    }
    return status::ok;
}

/**
 * @brief Shortest distance from `from` to `to` (hop count on unweighted graphs).
 * @return Distance, or `infinity<Graph>` when unreachable, a node is out of range, or a negative
 * weight is met.
 * @note Same engine as `shortest_paths()`, stopping once `to` is settled.
 */
template <typename Graph>
typename Graph::weight_type distance(CASTLE_CONST Graph& graph, size_type from, size_type to) CASTLE_NOEXCEPT
{
    shortest_path_tree<Graph> tree;
    static_cast<void>(detail::graph_search<Graph>::run(graph, from, to, tree));
    return tree.distance(to);
}

/**
 * @brief Writes one shortest path from `from` to `to`, both ends included.
 * @param path Cleared, then filled in `from`-to-`to` order.
 * @return `status::ok`; `status::out_of_range` for an invalid node; `status::not_found` when `to`
 * is unreachable; `status::invalid_argument` when a negative weight is met.
 * @note Same engine as `shortest_paths()`, stopping once `to` is settled.
 */
template <typename Graph>
status shortest_path(CASTLE_CONST Graph& graph, size_type from, size_type to, node_list<Graph>& path) CASTLE_NOEXCEPT
{
    path.clear();
    shortest_path_tree<Graph> tree;
    CASTLE_CONST status result = detail::graph_search<Graph>::run(graph, from, to, tree);
    if (result != status::ok)
    {
        return result;
    }
    return tree.path_to(to, path);
}

/**
 * @brief Minimum spanning tree (forest) of an undirected graph (Kruskal with union-find).
 * @param graph Undirected graph.
 * @param tree Cleared, then filled with the chosen edges in ascending weight order.
 * @return Total weight of the chosen edges (edge count for unweighted graphs).
 * @note A disconnected graph yields a spanning forest: `tree.size() == graph.size() - components`.
 * Equal weights are taken in edge slot order. Unweighted graphs skip the sort (every spanning
 * forest is minimal) and take edges by lower end node, then adjacency order. Self-loops are never
 * chosen. Complexity: O(E log E) weighted, O(V + E) unweighted. Stack: M (node, arc) index pairs.
 */
template <typename Graph>
typename Graph::weight_type minimum_spanning_tree(CASTLE_CONST Graph& graph, edge_list<Graph>& tree) CASTLE_NOEXCEPT
{
    static_assert(!Graph::directed, "graph_algorithms::minimum_spanning_tree requires an undirected graph");
    using weight_type = typename Graph::weight_type;
    using index_type = typename Graph::index_type;
    using arc_index_type = typename Graph::arc_index_type;

    struct candidate
    {
        index_type from;
        arc_index_type arc;
    };

    candidate candidates[Graph::static_edge_capacity];
    size_type count = 0U;
    for (size_type node = 0U; node < graph.size(); ++node)
    {
        for (size_type arc = graph.first_arc(node); arc != Graph::npos; arc = graph.next_arc(arc))
        {
            if (graph.target(arc) > node)
            {
                candidates[count].from = static_cast<index_type>(node);
                candidates[count].arc = static_cast<arc_index_type>(arc);
                ++count;
            }
        }
    }
    CASTLE_IF_CONSTEXPR (Graph::weighted)
    {
        castle::sort(candidates, candidates + count, [&graph](CASTLE_CONST candidate& lhs, CASTLE_CONST candidate& rhs) {
            CASTLE_CONST weight_type left = graph.weight(lhs.arc);
            CASTLE_CONST weight_type right = graph.weight(rhs.arc);
            return (left < right) || (!(right < left) && (lhs.arc < rhs.arc));
        });
    }

    tree.clear();
    weight_type total = static_cast<weight_type>(0);
    component_set<Graph> components(graph.size());
    for (size_type i = 0U; i < count; ++i)
    {
        if (components.unite(candidates[i].from, graph.target(candidates[i].arc)))
        {
            tree.push_back(detail::make_edge(graph, candidates[i].from, candidates[i].arc));
            total = static_cast<weight_type>(total + graph.weight(candidates[i].arc));
        }
    }
    return total;
}

/**
 * @brief Finds every bridge of an undirected graph (Tarjan low-link, iterative).
 * @param graph Undirected graph.
 * @param out Cleared, then filled with the bridges as `{parent, child}` DFS tree edges, in the
 * order their child subtree finishes.
 * @note A bridge is an edge whose removal disconnects its ends. Complexity: O(V + E).
 * Stack: about 5N indices plus an N-bit mask.
 */
template <typename Graph>
void bridges(CASTLE_CONST Graph& graph, edge_list<Graph>& out) CASTLE_NOEXCEPT
{
    out.clear();
    detail::low_link<Graph>(graph, &out, nullptr);
}

/**
 * @brief Finds every articulation point (cut vertex) of an undirected graph (Tarjan low-link).
 * @param graph Undirected graph.
 * @param out Cleared, then filled with the articulation points in ascending order.
 * @note An articulation point is a node whose removal increases the number of components.
 * Complexity: O(V + E). Stack: about 5N indices plus two N-bit masks.
 */
template <typename Graph>
void articulation_points(CASTLE_CONST Graph& graph, node_list<Graph>& out) CASTLE_NOEXCEPT
{
    detail::node_mask<Graph> cut_nodes;
    detail::low_link<Graph>(graph, nullptr, &cut_nodes);
    out.clear();
    for (size_type node = 0U; node < graph.size(); ++node)
    {
        if (cut_nodes.test(node))
        {
            out.push_back(static_cast<typename Graph::index_type>(node));
        }
    }
}

/**
 * @brief Colors an undirected graph so that adjacent nodes differ (Welsh-Powell greedy heuristic).
 * @param graph Undirected graph.
 * @param colors Cleared, then filled with one color in `0 .. result - 1` per node.
 * @return Number of colors used; at most the maximum degree plus one. Not guaranteed minimal.
 * @note Nodes are colored by descending degree (ties by index), each with the smallest color no
 * colored neighbor uses. Self-loops are ignored. Complexity: O(V log V + E + V * N / 32).
 * Stack: N node indices, N arc indices, an N-bit mask.
 */
template <typename Graph>
size_type greedy_coloring(CASTLE_CONST Graph& graph, color_map<Graph>& colors) CASTLE_NOEXCEPT
{
    static_assert(!Graph::directed, "graph_algorithms::greedy_coloring requires an undirected graph");
    using index_type = typename Graph::index_type;
    using arc_index_type = typename Graph::arc_index_type;
    CASTLE_CONST size_type nodes = graph.size();
    CASTLE_CONST index_type uncolored = static_cast<index_type>(Graph::static_capacity);

    index_type order[Graph::static_capacity];
    arc_index_type degree[Graph::static_capacity];
    colors.clear();
    for (size_type node = 0U; node < nodes; ++node)
    {
        order[node] = static_cast<index_type>(node);
        degree[node] = static_cast<arc_index_type>(graph.degree(node));
        colors.push_back(uncolored);
    }
    castle::sort(order, order + nodes, [&degree](index_type lhs, index_type rhs) {
        return (degree[lhs] > degree[rhs]) || ((degree[lhs] == degree[rhs]) && (lhs < rhs));
    });

    size_type color_count = 0U;
    detail::node_mask<Graph> taken;
    for (size_type i = 0U; i < nodes; ++i)
    {
        CASTLE_CONST size_type node = order[i];
        taken.reset();
        for (size_type arc = graph.first_arc(node); arc != Graph::npos; arc = graph.next_arc(arc))
        {
            CASTLE_CONST index_type color = colors[graph.target(arc)];
            if (color != uncolored)
            {
                taken.set(color);
            }
        }
        // At most nodes - 1 neighbors are colored, so a free color below `nodes` always exists.
        CASTLE_CONST size_type color = taken.find_first(false);
        colors[node] = static_cast<index_type>(color);
        if (color >= color_count)
        {
            color_count = color + 1U;
        }
    }
    return color_count;
}

/**
 * @brief Two-colors an undirected graph if it is bipartite (BFS).
 * @param graph Undirected graph.
 * @param colors Cleared, then filled with 0 or 1 per node; the lowest node of each component gets 0.
 * @return `true` when no edge joins two nodes of the same color; `false` otherwise (an odd cycle
 * or a self-loop exists; `colors` is then partial).
 * @note Complexity: O(V + E). Stack: N node indices.
 */
template <typename Graph>
bool bipartite_coloring(CASTLE_CONST Graph& graph, color_map<Graph>& colors) CASTLE_NOEXCEPT
{
    static_assert(!Graph::directed, "graph_algorithms::bipartite_coloring requires an undirected graph");
    using index_type = typename Graph::index_type;
    CASTLE_CONST size_type nodes = graph.size();
    CASTLE_CONST index_type uncolored = static_cast<index_type>(Graph::static_capacity);

    colors.clear();
    for (size_type node = 0U; node < nodes; ++node)
    {
        colors.push_back(uncolored);
    }

    index_type queue[Graph::static_capacity];
    for (size_type start = 0U; start < nodes; ++start)
    {
        if (colors[start] != uncolored)
        {
            continue;
        }
        colors[start] = 0U;
        queue[0U] = static_cast<index_type>(start);
        size_type head = 0U;
        size_type tail = 1U;
        while (head < tail)
        {
            CASTLE_CONST size_type node = queue[head];
            ++head;
            for (size_type arc = graph.first_arc(node); arc != Graph::npos; arc = graph.next_arc(arc))
            {
                CASTLE_CONST size_type next = graph.target(arc);
                if (colors[next] == uncolored)
                {
                    colors[next] = static_cast<index_type>(1U - colors[node]);
                    queue[tail] = static_cast<index_type>(next);
                    ++tail;
                }
                else if (colors[next] == colors[node])
                {
                    return false;
                }
            }
        }
    }
    return true;
}

} // namespace graph_algorithms

} // namespace dsa
} // namespace castle

#endif // CASTLE_DSA_GRAPH_HPP
