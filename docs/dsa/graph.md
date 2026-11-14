# graph

## Overview
Fixed-capacity graphs over the dense node indices `0 .. size() - 1`, plus a separate set of graph algorithms. The data structure only stores edges. The algorithms are free functions in `castle::dsa::graph_algorithms` that take any graph kind. Nothing is allocated and nothing recurses.

Four kinds are supported:

| Alias | Directed | Weighted |
| --- | --- | --- |
| `undirected_graph<N, M>` | no | no (every edge weighs 1) |
| `directed_graph<N, M>` | yes | no |
| `undirected_weighted_graph<N, M, W = int32_t>` | no | yes |
| `directed_weighted_graph<N, M, W = int32_t>` | yes | yes |

`N` is the node capacity and `M` the edge capacity. An undirected edge counts once toward `M`. The runtime node count `size()` can be anything from `0` to `N`, and every algorithm works on nodes `0 .. size() - 1` only.

Edges are stored as adjacency lists threaded through fixed arrays, so memory is O(N + M). Each node has a singly linked list of outgoing arcs in edge insertion order. An undirected edge uses two arcs, one per end. A directed edge uses one.

## Header
`#include "castle/dsa/graph.hpp"`

## Dependencies
- [compiler](../core/compiler.md)
- [error_handler](../core/error_handler.md)
- [traits](../core/traits.md) (`meta::smallest_uint_t`, `meta::is_signed`)
- [type_ranges](../core/type_ranges.md) (`numeric_limits` for `infinity`)
- [types](../core/types.md)
- [algorithm](algorithm.md) (`castle::sort` for Kruskal and Welsh-Powell)
- [array_view](array_view.md)
- [disjoint_set](disjoint_set.md) (components, cycle detection, Kruskal)
- [grid](grid.md) (`distance_matrix`)
- [initializer_list](initializer_list.md)
- [vector](vector.md) (`node_list`, `edge_list`, `color_map`)
- [status](../core/status.md)
- [bitset](../utility/bitset.md) (visited masks)
- [swap](../utility/swap.md)

## Public API
### `graph_edge<Weight>`
| API | Description |
| --- | --- |
| `graph_edge<W>` | Aggregate `{from, to, weight}`. Used to build graphs and returned by `minimum_spanning_tree()` and `bridges()`. |
| `graph_edge<void>` | Aggregate `{from, to}` for unweighted graphs. |

### `basic_graph<N, M, Directed, Weight>`
| API | Description |
| --- | --- |
| `index_type` | Smallest unsigned type that holds `N`. |
| `arc_index_type` | Smallest unsigned type that holds the arc capacity (`M` directed, `2M` undirected). |
| `weight_type` | `Weight`, or `size_type` for unweighted graphs. In that case `weight(arc)` is always 1. |
| `edge_type` | `graph_edge<Weight>`. |
| `static_capacity`, `static_edge_capacity`, `static_arc_capacity` | `N`, `M`, and the arc capacity. |
| `directed`, `weighted` | Compile-time kind flags. |
| `npos` | `size_type(-1)`. Means "no arc / no node". |
| `explicit basic_graph(size_type nodes = N)` | Empty graph with `nodes` nodes. Asserts and clamps when `nodes > N`. O(N + M). |
| `basic_graph(size_type nodes, initializer_list<edge_type> edges)` | Builds the graph from an edge list. A rejected edge triggers an assert. |
| `status assign(size_type nodes)` | Sets the node count and removes every edge. Returns `out_of_range` when `nodes > N` and leaves the graph unchanged. O(N). |
| `void clear()` | Removes every edge and keeps `size()`. O(N). |
| `size()`, `capacity()`, `empty()` | Node count, `N`, and `size() == 0`. |
| `edge_count()`, `edge_capacity()`, `full()` | Stored edges, `M`, and `edge_count() == M`. |
| `status add_edge(from, to)` | Unweighted graphs only. Returns `ok`, `out_of_range`, `already_exists`, or `full`. O(deg(from) + deg(to)). |
| `status add_edge(from, to, weight)` | Weighted graphs only. Same result codes. |
| `status add_edge(const edge_type&)` | Adds an edge from its aggregate. |
| `status add_edges(array_view<const edge_type>)` | Adds edges in order and stops at the first rejected one, returning its status. |
| `status remove_edge(from, to)` | Removes the edge (both arcs if undirected). Returns `ok`, `out_of_range`, or `not_found`. The freed slot is reused. O(deg(from) + deg(to)). |
| `bool has_edge(from, to)` | Edge lookup. O(deg(from)). |
| `size_type degree(node)` | Number of outgoing arcs. A self-loop counts twice when undirected. O(deg). |
| `neighbor_range neighbors(node)` | Range-`for` over `neighbor { node, weight }` in insertion order. |
| `find_arc(from, to)`, `first_arc(node)`, `next_arc(arc)` | Arc handles for low-level walks. They return `npos` when there is no arc. |
| `target(arc)`, `weight(arc)`, `edge_index(arc)` | Arc data. Both arcs of an undirected edge share one `edge_index`. |

### `graph_algorithms` result types
| Type | Description |
| --- | --- |
| `node_list<G>` | `vector<G::index_type, N>`. Holds traversal orders, paths, topological orders, and articulation points. |
| `color_map<G>` | Alias of `node_list<G>`. `colors[v]` is the color of `v`. |
| `edge_list<G>` | `vector<G::edge_type, N>`. Holds the spanning tree or the bridges. |
| `distance_matrix<G>` | `grid<G::weight_type, N, N>`, used by Floyd-Warshall. |
| `component_set<G>` | `disjoint_set<N>`. Query it with `group_count()`, `same_set()`, `groups()`. |
| `infinity<G>` | Largest `weight_type` value. Means "unreachable". |
| `shortest_path_tree<G>` | `size()`, `empty()`, `source()`, `reachable(v)`, `distance(v)`, `parent(v)`, `status path_to(v, node_list&)`. After a failed run it is empty, so every query returns its neutral value. |

### `graph_algorithms` functions
| Function | Kinds | Algorithm, complexity |
| --- | --- | --- |
| `size_type bfs(g, source, visitor)` | all | BFS. `visitor(node)` returns `void`, or `bool` where `false` stops. Returns the number of visits. O(V + E). |
| `size_type dfs(g, source, visitor)` | all | Iterative pre-order DFS in the same order as the recursive version. O(V + E). |
| `bool reachable(g, from, to)` | all | BFS with early stop. |
| `bool connected(g, a, b)` | all | Undirected: same component. Directed: each node reaches the other (strongly connected). |
| `component_set<G> connected_components(g)` | all | Union-find. Directed graphs give weakly connected components. O(V + E α). |
| `component_set<G> strongly_connected_components(g)` | directed | Iterative Tarjan. O(V + E). |
| `bool has_cycle(g)` | all | Undirected: union-find. Directed: Kahn. A self-loop counts as a cycle. |
| `status topological_sort(g, node_list&)` | directed | Kahn, non-recursive. Returns `invalid_argument` on a cycle. O(V + E). |
| `status shortest_paths(g, source, tree)` | all | BFS when unweighted, Dijkstra with an indexed heap when weighted. Returns `invalid_argument` on a negative arc. O((V + E) log V). |
| `status bellman_ford(g, source, tree)` | all | Allows negative weights. Returns `invalid_argument` on a reachable negative cycle. O(VE). |
| `status floyd_warshall(g, distance_matrix&)` | all | All pairs. Returns `invalid_argument` on a negative cycle. O(V³). |
| `weight_type distance(g, from, to)` | all | Shortest distance (hop count when unweighted) with early stop. Returns `infinity<G>` when there is no answer. |
| `status shortest_path(g, from, to, node_list&)` | all | One shortest path, ends included. Returns `out_of_range`, `not_found`, or `invalid_argument`. |
| `weight_type minimum_spanning_tree(g, edge_list&)` | undirected | Kruskal with union-find. Returns the total weight. A disconnected graph gives a spanning forest. O(E log E). |
| `void bridges(g, edge_list&)` | undirected | Tarjan low-link, iterative. O(V + E). |
| `void articulation_points(g, node_list&)` | undirected | Tarjan low-link, iterative. Output is in ascending order. O(V + E). |
| `size_type greedy_coloring(g, color_map&)` | undirected | Welsh-Powell: descending degree, then the smallest free color. Returns the number of colors used, at most Δ + 1. |
| `bool bipartite_coloring(g, color_map&)` | undirected | BFS two-coloring. Returns `false` on an odd cycle or a self-loop. |

Functions restricted to one kind reject the other kind at compile time with `static_assert`.

## Usage Example
```cpp
#include "castle/dsa/graph.hpp"

namespace ga = castle::dsa::graph_algorithms;

using harness = castle::dsa::undirected_weighted_graph<8U, 12U, uint16_t>;
const harness g(6U, { {0U, 1U, 40U}, {1U, 2U, 55U}, {0U, 3U, 60U},
                      {1U, 3U, 90U}, {2U, 4U, 70U}, {3U, 4U, 125U}, {4U, 5U, 30U} });

ga::edge_list<harness> wiring;
uint16_t length = ga::minimum_spanning_tree(g, wiring); // 255

ga::node_list<harness> run;
ga::shortest_path(g, 0U, 5U, run);                      // 0 1 2 4 5

ga::edge_list<harness> fragile;
ga::bridges(g, fragile);                                // {4, 5}

using boot = castle::dsa::directed_graph<6U, 8U>;
const boot deps(6U, { {0U, 1U}, {0U, 2U}, {1U, 3U}, {2U, 5U}, {3U, 4U}, {3U, 5U} });
ga::node_list<boot> order;
if (ga::topological_sort(deps, order) == castle::status::ok)
{
    // start services in `order`
}
```

See `samples/dsa/graph.cpp` for harness planning (MST, bridges, articulation points), boot sequencing (topological sort, cycle and SCC detection), energy routing with negative weights (Bellman-Ford, Floyd-Warshall), and channel assignment (coloring, components, BFS).

## Constraints & Notes
- No allocation, exceptions, RTTI, virtual dispatch, or recursion. Every DFS-based algorithm (DFS, Tarjan SCC, bridges, articulation points) uses an explicit frame stack with an arc cursor.
- Graph footprint: `N` head indices, plus `A` next indices and `A` target indices, plus `A` weights when weighted (`A = M` directed, `2M` undirected), plus four counters. All of them use the compact index types. `undirected_graph<10, 20>` takes 94 bytes.
- Algorithm scratch lives on the caller's stack and is sized by the graph's capacities, not by `size()`. Typical cost is N to 6N compact indices plus N-bit masks. Kruskal also needs M (node, arc) pairs. The exact figure is in each function's doxygen. Choose `N` and `M` with the stack budget in mind.
- Results are written to caller-owned objects (`node_list`, `edge_list`, `color_map`, `shortest_path_tree`, `distance_matrix`), and these are sized for the worst case. The only exception is `component_set`, a `disjoint_set` returned by value. `distance_matrix` holds N² weights, so place it statically when N is large.
- Parallel edges are rejected with `already_exists`. Self-loops are accepted: they count as cycles, are never bridges or MST edges, make a graph non-bipartite, and are ignored by `greedy_coloring()`.
- Removed edge slots go on a free list and are reused, so `add_edge()` and `remove_edge()` never fragment capacity.
- Neighbor order is insertion order. BFS, DFS, topological ties, and Kruskal ties are therefore deterministic and follow the order of the edge list.
- Weights: Dijkstra (`shortest_paths()`, `distance()`, `shortest_path()`) requires non-negative weights and reports a negative arc only when it scans one. Use `bellman_ford()` or `floyd_warshall()` with negative weights. An undirected negative edge is itself a negative cycle. Sums are computed in `weight_type`, so pick a type wide enough for path lengths. `infinity<G>` (the type's maximum) is reserved for "unreachable".
- Unweighted graphs report `weight_type = size_type` with every weight equal to 1, so the same algorithms return hop counts.
- Node arguments `>= size()` are rejected in every build. Mutators return `out_of_range`, and queries return `false`, `0`, `npos`, `infinity<G>`, or an empty range/tree. Arc handles passed to `next_arc()`, `target()`, `weight()`, and `edge_index()` must be live and are not checked.
- Graph objects are trivially copyable, and copies are independent. Not thread-safe: concurrent mutation needs external synchronization, while concurrent const queries and algorithms on an unchanging graph are safe.

## Algorithm Design
This section explains how `basic_graph` stores a graph and how the algorithms run on it. It assumes you know the textbook version: an adjacency map such as `vector<vector<int>>`, BFS and DFS, and Dijkstra with `std::priority_queue`. Each part starts from that version and shows what changes and why.

One point up front: the design is **not asymptotically faster** than the textbook version. BFS is still O(V + E) and Dijkstra is still O((V + E) log V). What it gains is a footprint known at compile time, no heap, no recursion, and timing that does not depend on an allocator. Those are the properties an embedded target needs.

### 1. From an adjacency map to one shared pool
The textbook graph looks like this:

```cpp
std::vector<std::vector<int>> adj(n);   // one growable list per node
adj[u].push_back(v);                    // add edge u -> v
for (int w : adj[u]) { /* ... */ }      // visit the neighbors of u
```

On a microcontroller this has four problems:

1. Each inner `vector` is a separate heap block. Growing it reallocates and copies, and the allocator can fragment or fail at run time.
2. The worst-case memory is unknown at compile time, so it cannot be proven to fit.
3. Each node pays for a vector header (24 bytes on a 64-bit host) even when it has no neighbor, and each neighbor costs a full `int`.
4. `push_back` is only *amortized* O(1). One call in many takes much longer, which breaks timing guarantees.

`basic_graph` keeps the idea of "one list per node" but changes where the list entries live. Every neighbor entry of every node goes into **one shared fixed array** (the pool). Each node keeps only the index of its first entry, and each entry stores the index of the next entry of the same node. In other words, every node owns a singly linked list, but the links are array indices instead of pointers, and the nodes of all lists share one array.

This layout is a well-known technique, often called a *linked forward star* or *chained adjacency list*. It is common in competitive programming because it needs no allocation at all.

### 2. Vocabulary: node, edge, arc, slot
| Term | Meaning |
| --- | --- |
| node | A vertex, identified by an index `0 .. size() - 1`. |
| edge | What you add with `add_edge()`. It counts toward `M`. |
| arc | One directed entry in a node's list: "from this node you can go to `target`". |
| slot | The storage place of one edge. Slots are numbered `0 .. M - 1`. |

A **directed** edge `u -> v` needs one arc, stored in the list of `u`. An **undirected** edge `u - v` needs two arcs: `u -> v` in the list of `u` and `v -> u` in the list of `v`. This matches the textbook version, where an undirected edge is pushed into both `adj[u]` and `adj[v]`.

The arc index is derived from the slot index, so it is never stored:

| Graph | Arcs of slot `s` | `edge_index(arc)` | Twin of `arc` |
| --- | --- | --- | --- |
| directed | `s` | `arc` | none |
| undirected | `2s` (from -> to) and `2s + 1` (to -> from) | `arc / 2` | `arc ^ 1` |

The two arcs of an undirected edge sit next to each other, so `arc ^ 1` (flip the lowest bit) jumps from one arc to its twin in O(1). `remove_edge()` uses this to unlink both arcs, and the bridge algorithm uses it to recognize the edge it arrived through.

### 3. Storage layout with a worked example
The graph keeps three arrays (four when weighted). `A` is the arc capacity: `M` for directed graphs, `2M` for undirected graphs.

| Array | Size | Content |
| --- | --- | --- |
| `head_[node]` | `N` | First arc in the list of `node`, or "none". |
| `next_[arc]` | `A` | Next arc in the same list, or "none". |
| `target_[arc]` | `A` | Node the arc points to. |
| `weights_[arc]` | `A` | Arc weight. Only present when weighted. |

Take an undirected graph with 4 nodes and the edges added in this order: `0-1`, `0-2`, `1-2`, `2-3`.

```text
    0 ----- 1
     \     /
      \   /
        2 ----- 3
```

Each edge takes the next slot and the two arcs of that slot:

| Slot | Edge | Arc `2s` | Arc `2s + 1` |
| --- | --- | --- | --- |
| 0 | `0-1` | arc 0: `0 -> 1` | arc 1: `1 -> 0` |
| 1 | `0-2` | arc 2: `0 -> 2` | arc 3: `2 -> 0` |
| 2 | `1-2` | arc 4: `1 -> 2` | arc 5: `2 -> 1` |
| 3 | `2-3` | arc 6: `2 -> 3` | arc 7: `3 -> 2` |

The arrays end up as follows (`-` is "none"):

```text
node        0   1   2   3
head_    [  0,  1,  3,  7 ]

arc         0   1   2   3   4   5   6   7
target_  [  1,  0,  2,  0,  2,  1,  3,  2 ]
next_    [  2,  4,  -,  5,  -,  6,  -,  - ]
```

To list the neighbors of node 2, start at `head_[2] = 3` and follow `next_`:

```text
arc 3 -> target 0,  next_[3] = 5
arc 5 -> target 1,  next_[5] = 6
arc 6 -> target 3,  next_[6] = -   (end)
```

Node 2's neighbors are `0, 1, 3`, exactly what `adj[2]` would hold in the textbook version, in the same order the edges were added.

### 4. Walking the neighbors
These two loops do the same work:

```cpp
// textbook
for (int w : adj[u]) { use(w); }

// basic_graph, low level (what every algorithm in this header uses)
for (size_type arc = g.first_arc(u); arc != G::npos; arc = g.next_arc(arc))
{
    use(g.target(arc), g.weight(arc));
}

// basic_graph, convenience range
for (auto nb : g.neighbors(u)) { use(nb.node, nb.weight); }
```

The algorithms use the arc form because the arc handle carries extra information: `edge_index(arc)` names the edge, and `arc ^ 1` names its twin.

A linked list is usually less cache-friendly than a contiguous vector. Here the effect is small, because the whole graph is one small block: `undirected_graph<10, 20>` is 94 bytes, which is about two cache lines.

### 5. Adding an edge
`add_edge(u, v)` does the following:

1. Walk the list of `u` once. This one walk does two jobs: it rejects a duplicate (`already_exists`) and it finds the last arc (the tail).
2. Take a free slot (see section 6) and fill its arc(s): set `target_`, the weight, and `next_ = none`.
3. Append the arc after the tail, or make it `head_[u]` when the list was empty.
4. For an undirected graph, append the twin arc `v -> u` at the end of `v`'s list in the same way.

The cost is O(deg(u) + deg(v)), not O(1). That is the price of two features: duplicate edges are rejected, and neighbors stay in insertion order. Insertion order makes every algorithm deterministic, because the visit order follows the order of your edge list. Prepending at the head would be O(1) but would reverse that order.

### 6. Removing an edge, and the free list
`remove_edge(u, v)` finds the arc `u -> v`, unlinks it from `u`'s list (the usual "previous.next = current.next" step), unlinks the twin `arc ^ 1` from `v`'s list, and frees the slot.

A freed slot leaves a hole in the pool. If new edges only ever took never-used slots, a long run of add/remove cycles would use up all `M` slots. The graph therefore keeps a **free list** of released slots, and it needs no extra array for it: a free slot's arcs are not in any node's list, so their `next_` entry is unused and can chain the free slots together.

- `free_slot_` is the first free slot, or "none".
- `release(s)` pushes `s` on the front: `next_[first arc of s] = free_slot_`, then `free_slot_ = s`.
- `acquire()` pops from the free list first. When the free list is empty, it takes the next never-used slot (`used_slots_++`).

Continuing the example, `remove_edge(0, 2)` changes the arrays like this:

```text
find_arc(0, 2):      walk node 0: arc 0 (-> 1), arc 2 (-> 2)  => arc 2
unlink arc 2 from 0: next_[0] = next_[2] = -     node 0: arc 0
unlink arc 3 from 2: head_[2] = next_[3] = 5     node 2: arc 5, arc 6
release slot 1:      next_[2] = -, free_slot_ = 1
```

A following `add_edge(1, 3)` reuses slot 1 (arcs 2 and 3) instead of needing a fifth slot. Every operation is a few index writes, so add and remove never allocate, never fragment, and never exceed the capacity.

### 7. Compact index types and "none" markers
Indices are stored in the smallest unsigned type that fits:

- `index_type = smallest_uint_t<N>` for node indices (`target_`, node lists, queues).
- `arc_index_type = smallest_uint_t<A>` for arc indices (`head_`, `next_`).

With `N <= 255` and `A <= 255`, every entry is one byte, where the textbook version spends an `int` (4 bytes) per neighbor.

"None" needs a value that is never a real index. The largest real arc index is `A - 1`, so the value `A` itself is the "no arc" marker, and `N` is the "no node" marker. That is why the types are chosen to hold `N` and `A`, not just `N - 1` and `A - 1`. The public API converts the marker to `npos` (`size_type(-1)`), so callers never see the compact form.

Footprint of `undirected_graph<10, 20>` (A = 40, all one-byte indices):

| Part | Bytes |
| --- | --- |
| `head_` | 10 |
| `next_` | 40 |
| `target_` | 40 |
| four counters | 4 |
| **total** | **94** |

The same graph as `vector<vector<int>>` needs an outer vector, ten inner vector headers, ten separate heap blocks, and the allocator's bookkeeping for each block. On a typical 64-bit host that comes to several hundred bytes. The exact figure depends on the standard library's growth policy and the allocator.

### 8. Weights cost nothing when unused
The weight array lives in a small base class, `detail::graph_weight_store<Weight, A>`, that `basic_graph` inherits privately.

- For an arithmetic `Weight`, the base holds `Weight weights_[A]`.
- For `void`, the specialization has no data members, and `stored_weight()` always returns 1. The compiler's empty-base optimization gives it zero bytes.

Unweighted graphs therefore pay nothing for weights. Because every unweighted arc reports weight 1, the same algorithms return hop counts on unweighted graphs and real distances on weighted graphs, with no second code path in the graph itself.

### 9. Cost comparison
| Operation | `vector<vector<int>>` | `unordered_map<int, unordered_set<int>>` | `basic_graph` |
| --- | --- | --- | --- |
| Add edge | Amortized O(1). May allocate. Accepts duplicates. | Average O(1). Allocates, may rehash. | O(deg(u) + deg(v)). Never allocates. Rejects duplicates. |
| Remove edge | O(deg). The erase shifts elements. | Average O(1). | O(deg(u) + deg(v)). The slot is reused. |
| `has_edge` | O(deg) | Average O(1) | O(deg) |
| Visit neighbors | O(deg), contiguous | O(deg), scattered | O(deg), inside one small block |
| Memory | Unknown at compile time, heap | Unknown at compile time, heap, large overhead | `sizeof` is a compile-time constant |
| Worst-case time | Allocation spikes | Rehash spikes | Bounded by the degree |

### 10. How the algorithms are built
Every algorithm follows the same three rules:

- **Scratch is local arrays sized by the capacity**, for example `index_type queue[N]`. The compiler knows the size, so the stack cost is fixed. It is sized by `N` (or `M`), not by `size()`, because C++ arrays need a compile-time size.
- **"Visited" is a bitset** (`bitset<N>`): one bit per node instead of a `bool` or a `std::set`.
- **Nothing recurses.** Recursion depth on a long path graph would be `N` frames of unknown size. Every DFS is written with an explicit stack whose size is known.

#### BFS (`bfs`, `reachable`, unweighted shortest paths, `bipartite_coloring`)
This is the textbook BFS with one change: the queue is a plain array of `N` entries with `head` and `tail` counters. Each node is enqueued at most once, because it is marked visited when it is pushed, so `tail` never exceeds `N`. The queue never has to wrap around and needs no ring-buffer logic. Popping is `++head`.

On an unweighted graph BFS already *is* the shortest-path algorithm: nodes leave the queue in order of hop distance. `shortest_paths()` therefore uses BFS when the graph is unweighted and Dijkstra only when it is weighted.

#### DFS without recursion: the arc cursor (`dfs`, Tarjan SCC, bridges, articulation points)
The usual iterative DFS pushes every neighbor:

```cpp
stack.push(s);
while (!stack.empty()) {
    int v = stack.top(); stack.pop();
    if (visited[v]) continue;
    visited[v] = true; visit(v);
    for (int w : adj[v]) stack.push(w);   // pushes ALL neighbors
}
```

This version has three drawbacks:

- The stack can hold up to E entries, because a node is pushed once per incoming arc.
- The visit order differs from recursive DFS: neighbors come out reversed.
- It has no "finished with `v`" moment. Tarjan's algorithms need that moment, because they compute a value from a node's children after those children are done.

Recursive DFS has none of these problems because each call keeps its own loop position:

```cpp
void dfs(int v) {
    visited[v] = true; enter(v);
    for (int w : adj[v])                  // <- the loop position lives in this frame
        if (!visited[w]) dfs(w); else revisit(v, w);
    leave(v);
}
```

`detail::dfs_walk` rebuilds exactly that with an explicit stack. Each frame stores three small values:

| Frame field | Recursive equivalent |
| --- | --- |
| `nodes[d]` | the `v` of the call at depth `d` |
| `cursor[d]` | the loop position: the next arc of `v` still to try |
| `entry[d]` | the arc that led into `v` (for the bridge twin check) |

Each step looks at the top frame:

- If its cursor is "none", every arc is done: call `leave(v, parent, entry)` and pop.
- Otherwise, advance the cursor by one arc. If the target is already visited, call `revisit(...)`. If not, call `enter(...)` and push a new frame for it.

Each node is entered at most once, so the depth never exceeds `N`. The stack is exactly `N` frames, and the visit order is identical to the recursive version.

The three calls `enter`, `revisit`, and `leave` are a "hooks" object. Plain DFS uses hooks that just call your visitor. Tarjan SCC and the bridge / articulation search plug in their own hooks. One walk engine serves all of them, which avoids duplicating subtle stack code.

#### Dijkstra with an indexed heap (`shortest_paths`, `distance`, `shortest_path`)
The textbook version uses *lazy deletion*:

```cpp
pq.push({0, s});
while (!pq.empty()) {
    auto [d, v] = pq.top(); pq.pop();
    if (d > dist[v]) continue;            // stale entry, skip
    for (auto [w, c] : adj[v])
        if (dist[v] + c < dist[w]) { dist[w] = dist[v] + c; pq.push({dist[w], w}); }
}
```

Each improvement pushes a new `(distance, node)` pair and leaves the old pair in the heap as garbage. In the worst case the heap holds about E entries, so its size depends on the edge count and is awkward to bound in a fixed array.

`detail::graph_search::min_queue` is an **indexed binary heap** with *decrease-key* instead:

- `heap_[]` holds node indices only. The key is not copied. It is read from `tree.distance_[node]`, so the distance exists in one place only.
- `position_[node]` records where `node` sits in `heap_`, or "absent".
- `push_or_decrease(v)` checks `position_[v]`. If `v` is absent, it is appended at the end. Either way it then *sifts up* from its slot. When `dist[v]` just decreased, sifting up from its current slot restores heap order, so no second copy is needed.
- `pop()` takes the root, moves the last element to the root, and sifts down. Every swap updates `position_` for both nodes.

Example: the heap holds nodes 2, 0, 3, and then `dist[3]` drops below `dist[2]`:

```text
before:  heap_ = [2, 0, 3]     position_ = [1, -, 0, 2]
relax 3: dist[3] lowered;  position_[3] = 2 -> sift up from slot 2
         parent slot (2-1)/2 = 0 holds node 2, dist[3] < dist[2] -> swap
after:   heap_ = [3, 0, 2]     position_ = [1, -, 2, 0]
```

Each node is in the heap at most once, so the heap never holds more than `size() <= N` entries. Two arrays of `N` compact indices are enough. With `N <= 255` that is 2N bytes, against up to about E entries of 8 to 16 bytes each for the lazy `(distance, node)` pairs. The complexity stays O((V + E) log V).

Two more details:

- **Early stop.** `distance(g, from, to)` and `shortest_path()` stop when `to` is *popped*, not when it is first reached. A popped node's distance is final in Dijkstra, while a reached node's distance may still improve.
- **Negative weights** break Dijkstra's "popped means final" rule. The search returns `invalid_argument` when it scans a negative arc. It only checks the arcs it scans, so a negative arc in an unreachable part of the graph is not reported.

#### Bellman-Ford (`bellman_ford`)
This is the textbook round-based relaxation, with two changes:

- **Early exit.** If a full round changes nothing, every distance is final and the function returns at once. Typical inputs need far fewer than V rounds.
- **Negative-cycle proof.** Without a negative cycle, every shortest path has at most V - 1 edges, so V - 1 rounds suffice. If round V still improves something, a negative cycle is reachable and the result is `invalid_argument`. In an undirected graph, a negative edge `u - v` is itself such a cycle (`u -> v -> u`).

Nodes still at infinity are skipped. This saves work and also prevents `infinity + weight` from overflowing.

#### Floyd-Warshall (`floyd_warshall`)
This is the textbook triple loop `dist[i][j] = min(dist[i][j], dist[i][k] + dist[k][j])` over a caller-owned `grid<weight, N, N>`. Like Bellman-Ford it skips infinite legs: when `dist[i][k]` is infinite the whole inner loop is skipped. That speeds up sparse graphs and avoids overflow, because infinity is the type's maximum value. A negative diagonal entry afterwards means a negative cycle.

#### Union-find (`connected_components`, undirected `has_cycle`, `minimum_spanning_tree`)
A `disjoint_set<N>` (union by rank, path compression, near-O(1) per operation) answers "are these two nodes already in the same group?".

- **Components:** unite the two ends of every arc. Directed graphs give *weakly* connected components, because direction is ignored.
- **Undirected cycle:** process each edge once, from its lower end, so the twin arc is not counted again. If `unite` reports that both ends were already in one group, this edge closes a cycle. A self-loop unites a node with itself and is reported as a cycle.
- **Kruskal:** collect each undirected edge once as a compact `(from, arc)` pair (1 + 1 bytes for small graphs). Sort the pairs by weight, with ties broken by arc index for a deterministic result. Then take every edge that joins two different groups. Unweighted graphs skip the sort, because every spanning forest of an unweighted graph has the same total.

#### Topological sort, Kahn's algorithm (`topological_sort`, directed `has_cycle`)
Count the in-degree of every node and start with the nodes whose in-degree is 0. Repeatedly take one, append it to the order, and decrement the in-degree of its targets; a target that reaches 0 becomes ready.

The output vector doubles as the queue: entries before position `i` are done, and entries from `i` to the end are ready but not yet processed. No separate queue array is needed. If the final order has fewer than `size()` nodes, the remaining nodes lie on or behind a cycle. `has_cycle()` on a directed graph is exactly this check.

#### Tarjan low-link (`strongly_connected_components`, `bridges`, `articulation_points`)
All three run on the DFS engine above and use two numbers per node:

- `order[v]` is the DFS discovery time of `v`.
- `low[v]` is the smallest discovery time reachable from `v`'s DFS subtree when you may use **one** non-tree arc at the end. It starts as `order[v]`. A `revisit` arc lowers it to `order[target]`, and on `leave` a child passes its `low` up to its parent.

The intuition: `low[child]` says how high up the DFS tree the child's subtree can climb without using the tree edge `parent - child`.

| Question | Test on `leave(child)` | Reason |
| --- | --- | --- |
| Is `parent - child` a **bridge**? | `low[child] > order[parent]` | The subtree cannot reach `parent` or anything above it except through this edge. |
| Is `parent` an **articulation point** (non-root)? | `low[child] >= order[parent]` | Removing `parent` cuts the subtree off from the rest. |
| Is the DFS root an articulation point? | It has 2 or more DFS children. | The root has no ancestors, so the child test does not apply. |

Twin-arc check: in an undirected graph, the list of `child` contains the twin arc back to `parent`. If that arc counted as a back edge, `low[child]` would always reach `order[parent]`, and no bridge would ever be found. The hook skips the arc whose `edge_index` equals that of the entry arc. It identifies the *edge*, not the parent node, which is the correct rule in general and cheap here because `edge_index` is just `arc / 2`.

For **strongly connected components** (directed graphs), each discovered node also goes onto a separate node stack. A `revisit` arc lowers `low` only when its target is still on that stack, because a target in an already finished component belongs to a different SCC. When `leave(v)` finds `low[v] == order[v]`, `v` is the first node discovered in its component: pop the node stack down to `v` and unite all of those nodes into one group.

#### Coloring (`greedy_coloring`, `bipartite_coloring`)
- **Welsh-Powell:** sort nodes by descending degree, with ties broken by index. Give each node the smallest color no colored neighbor uses. The "used colors" set is a `bitset<N>` that is cleared per node, and `find_first(false)` returns the smallest free color. The result is at most max-degree + 1 colors, but it is not guaranteed minimal; minimal coloring is NP-hard.
- **Bipartite:** BFS from every uncolored node, giving each newly reached node the opposite color of its parent. An arc between two nodes of the same color means an odd cycle, and the graph is not bipartite.

### 11. When the textbook version is the better choice
The trade-offs are deliberate. Prefer a heap-based adjacency map when:

- The node or edge count has no useful upper bound, or the upper bound is far above the typical size. Capacity here is reserved up front, and algorithm scratch is sized by `N`, not by `size()`.
- You need many `has_edge()` lookups on nodes with high degree. A hash set per node gives O(1); this list gives O(deg).
- You add many edges to one node with very high degree. Each `add_edge()` walks that node's list.
- Nodes are named by sparse keys (strings, IDs) rather than dense indices `0 .. n - 1`.

For embedded workloads with known small bounds (wiring harnesses, boot dependency graphs, routing tables, channel plans), the fixed pool gives the same algorithmic complexity with a memory budget that can be checked at compile time.
