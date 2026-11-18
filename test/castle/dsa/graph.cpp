#include <gtest/gtest.h>

#include "castle/dsa/graph.hpp"

#include <stdint.h>

namespace
{
using castle::size_type;
using castle::status;
using castle::dsa::directed_graph;
using castle::dsa::directed_weighted_graph;
using castle::dsa::undirected_graph;
using castle::dsa::undirected_weighted_graph;
namespace ga = castle::dsa::graph_algorithms;

template <typename Graph>
bool is_proper_coloring(const Graph& graph, const ga::color_map<Graph>& colors, size_type color_count)
{
    if (colors.size() != graph.size())
    {
        return false;
    }
    for (size_type node = 0U; node < graph.size(); ++node)
    {
        if (colors[node] >= color_count)
        {
            return false;
        }
        for (auto adjacent : graph.neighbors(node))
        {
            if ((adjacent.node != node) && (colors[adjacent.node] == colors[node]))
            {
                return false;
            }
        }
    }
    return true;
}

template <typename Graph>
size_type position_of(const ga::node_list<Graph>& order, size_type node)
{
    for (size_type i = 0U; i < order.size(); ++i)
    {
        if (order[i] == node)
        {
            return i;
        }
    }
    return Graph::npos;
}

// ---------------------------------------------------------------------------
// Data structure
// ---------------------------------------------------------------------------

TEST(GraphTest, CompileTimeTraits)
{
    using small = undirected_graph<10U, 20U>;
    static_assert(sizeof(small::index_type) == 1U, "10 nodes fit uint8_t");
    static_assert(sizeof(small::arc_index_type) == 1U, "40 arcs fit uint8_t");
    static_assert(sizeof(undirected_graph<10U, 200U>::arc_index_type) == 2U, "400 arcs need uint16_t");
    static_assert(sizeof(directed_graph<10U, 200U>::arc_index_type) == 1U, "200 directed arcs fit uint8_t");
    static_assert(sizeof(directed_graph<256U, 4U>::index_type) == 2U, "256 nodes need uint16_t");
    static_assert(small::static_capacity == 10U, "node capacity");
    static_assert(small::static_edge_capacity == 20U, "edge capacity");
    static_assert(small::static_arc_capacity == 40U, "undirected edges use two arcs");
    static_assert(directed_graph<4U, 7U>::static_arc_capacity == 7U, "directed edges use one arc");
    static_assert(!small::directed && !small::weighted, "undirected unweighted");
    static_assert(directed_weighted_graph<4U, 4U>::directed && directed_weighted_graph<4U, 4U>::weighted,
                  "directed weighted");
    static_assert(castle::meta::is_same<small::weight_type, size_type>::value, "unweighted weight type");
    static_assert(castle::meta::is_same<undirected_weighted_graph<4U, 4U>::weight_type, int32_t>::value,
                  "default weight type");
    static_assert(castle::meta::is_same<undirected_weighted_graph<4U, 4U, float>::edge_type,
                                        castle::dsa::graph_edge<float>>::value,
                  "weighted edge type");

    // Unweighted graphs store no weights: 10 heads + 40 next + 40 targets + 4 counters.
    EXPECT_EQ(sizeof(small), 94U);
    EXPECT_GT(sizeof(undirected_weighted_graph<10U, 20U, uint16_t>), sizeof(small) + 40U * sizeof(uint16_t) - 1U);
}

TEST(GraphTest, ConstructionAssignAndClear)
{
    undirected_graph<6U, 8U> graph;
    EXPECT_EQ(graph.size(), 6U);
    EXPECT_EQ(graph.capacity(), 6U);
    EXPECT_EQ(graph.edge_capacity(), 8U);
    EXPECT_EQ(graph.edge_count(), 0U);
    EXPECT_FALSE(graph.empty());
    EXPECT_FALSE(graph.full());

    undirected_graph<6U, 8U> partial(3U);
    EXPECT_EQ(partial.size(), 3U);
    EXPECT_EQ(partial.add_edge(1U, 2U), status::ok);
    EXPECT_EQ(partial.add_edge(2U, 3U), status::out_of_range);

    EXPECT_EQ(partial.assign(7U), status::out_of_range);
    EXPECT_EQ(partial.size(), 3U);
    EXPECT_TRUE(partial.has_edge(1U, 2U));

    EXPECT_EQ(partial.assign(5U), status::ok);
    EXPECT_EQ(partial.size(), 5U);
    EXPECT_EQ(partial.edge_count(), 0U);
    EXPECT_FALSE(partial.has_edge(1U, 2U));
    EXPECT_EQ(partial.add_edge(3U, 4U), status::ok);

    partial.clear();
    EXPECT_EQ(partial.size(), 5U);
    EXPECT_EQ(partial.edge_count(), 0U);
    EXPECT_FALSE(partial.has_edge(3U, 4U));
    EXPECT_EQ(partial.degree(3U), 0U);

    EXPECT_EQ(partial.assign(0U), status::ok);
    EXPECT_TRUE(partial.empty());
    EXPECT_EQ(partial.add_edge(0U, 0U), status::out_of_range);
}

TEST(GraphTest, InitializerListBuildsEveryKind)
{
    undirected_graph<4U, 4U> undirected(4U, {{0U, 1U}, {1U, 2U}});
    directed_graph<4U, 4U> directed(4U, {{0U, 1U}, {1U, 2U}});
    undirected_weighted_graph<4U, 4U> undirected_weighted(4U, {{0U, 1U, 5}, {1U, 2U, -2}});
    directed_weighted_graph<4U, 4U, float> directed_weighted(3U, {{0U, 1U, 0.5F}, {2U, 0U, 1.5F}});

    EXPECT_EQ(undirected.edge_count(), 2U);
    EXPECT_TRUE(undirected.has_edge(2U, 1U));
    EXPECT_EQ(directed.edge_count(), 2U);
    EXPECT_FALSE(directed.has_edge(2U, 1U));
    EXPECT_EQ(undirected_weighted.weight(undirected_weighted.find_arc(2U, 1U)), -2);
    EXPECT_EQ(directed_weighted.size(), 3U);
    EXPECT_FLOAT_EQ(directed_weighted.weight(directed_weighted.find_arc(2U, 0U)), 1.5F);
    EXPECT_EQ(directed_weighted.find_arc(0U, 2U), directed_weighted.npos);
}

TEST(GraphTest, AddEdgeReportsEveryOutcome)
{
    undirected_graph<4U, 3U> graph;
    EXPECT_EQ(graph.add_edge(0U, 1U), status::ok);
    EXPECT_EQ(graph.add_edge(4U, 1U), status::out_of_range);
    EXPECT_EQ(graph.add_edge(1U, 4U), status::out_of_range);
    EXPECT_EQ(graph.add_edge(0U, 1U), status::already_exists);
    EXPECT_EQ(graph.add_edge(1U, 0U), status::already_exists);
    EXPECT_EQ(graph.add_edge(1U, 2U), status::ok);
    EXPECT_EQ(graph.add_edge(castle::dsa::graph_edge<void>{2U, 3U}), status::ok);
    EXPECT_TRUE(graph.full());
    EXPECT_EQ(graph.add_edge(0U, 3U), status::full);
    EXPECT_EQ(graph.add_edge(0U, 1U), status::already_exists);
    EXPECT_EQ(graph.edge_count(), 3U);

    directed_graph<3U, 3U> directed;
    EXPECT_EQ(directed.add_edge(0U, 1U), status::ok);
    EXPECT_EQ(directed.add_edge(1U, 0U), status::ok);
    EXPECT_EQ(directed.add_edge(1U, 0U), status::already_exists);
    EXPECT_TRUE(directed.has_edge(0U, 1U));
    EXPECT_TRUE(directed.has_edge(1U, 0U));
    EXPECT_FALSE(directed.has_edge(0U, 2U));
    EXPECT_FALSE(directed.has_edge(3U, 0U));
    EXPECT_FALSE(directed.has_edge(0U, 3U));

    directed_weighted_graph<3U, 3U, uint8_t> weighted;
    EXPECT_EQ(weighted.add_edge(0U, 2U, 9U), status::ok);
    EXPECT_EQ(weighted.weight(weighted.find_arc(0U, 2U)), 9U);
}

TEST(GraphTest, AddEdgesStopsAtFirstRejectedEdge)
{
    using graph_type = undirected_weighted_graph<4U, 4U>;
    const graph_type::edge_type edges[] = {{0U, 1U, 1}, {1U, 2U, 2}, {2U, 1U, 3}, {2U, 3U, 4}};
    graph_type graph;
    EXPECT_EQ(graph.add_edges(castle::dsa::make_array_view(edges, 2U)), status::ok);
    graph.clear();
    EXPECT_EQ(graph.add_edges(castle::dsa::make_array_view(edges, 4U)), status::already_exists);
    EXPECT_EQ(graph.edge_count(), 2U);
    EXPECT_FALSE(graph.has_edge(2U, 3U));
    EXPECT_EQ(graph.add_edges(castle::dsa::array_view<const graph_type::edge_type>()), status::ok);
}

TEST(GraphTest, NeighborsFollowInsertionOrder)
{
    undirected_weighted_graph<5U, 6U, int16_t> graph(5U, {{0U, 3U, 30}, {0U, 1U, 10}, {2U, 0U, 20}});

    size_type nodes[3] = {};
    int16_t weights[3] = {};
    size_type count = 0U;
    for (auto adjacent : graph.neighbors(0U))
    {
        nodes[count] = adjacent.node;
        weights[count] = adjacent.weight;
        ++count;
    }
    ASSERT_EQ(count, 3U);
    EXPECT_EQ(nodes[0], 3U);
    EXPECT_EQ(nodes[1], 1U);
    EXPECT_EQ(nodes[2], 2U);
    EXPECT_EQ(weights[0], 30);
    EXPECT_EQ(weights[1], 10);
    EXPECT_EQ(weights[2], 20);
    EXPECT_EQ(graph.degree(0U), 3U);
    EXPECT_EQ(graph.degree(2U), 1U);
    EXPECT_EQ(graph.degree(9U), 0U);

    EXPECT_TRUE(graph.neighbors(4U).empty());
    EXPECT_TRUE(graph.neighbors(9U).empty());
    EXPECT_FALSE(graph.neighbors(1U).empty());
    EXPECT_TRUE(graph.neighbors(4U).begin() == graph.neighbors(4U).end());

    auto range = graph.neighbors(1U);
    auto it = range.begin();
    EXPECT_TRUE(it != range.end());
    EXPECT_EQ(it.arc(), graph.find_arc(1U, 0U));
    EXPECT_EQ((*it).node, 0U);
    ++it;
    EXPECT_TRUE(it == range.end());
    EXPECT_EQ(it.arc(), graph.npos);

    undirected_weighted_graph<5U, 6U, int16_t> copy = graph;
    EXPECT_TRUE(copy.neighbors(1U).begin() != graph.neighbors(1U).begin());
}

TEST(GraphTest, ArcHandlesOfUndirectedEdgeShareSlot)
{
    undirected_graph<3U, 3U> graph(3U, {{0U, 1U}, {1U, 2U}});
    const size_type forward = graph.find_arc(0U, 1U);
    const size_type backward = graph.find_arc(1U, 0U);
    EXPECT_NE(forward, backward);
    EXPECT_EQ(graph.edge_index(forward), graph.edge_index(backward));
    EXPECT_NE(graph.edge_index(forward), graph.edge_index(graph.find_arc(1U, 2U)));
    EXPECT_EQ(graph.target(forward), 1U);
    EXPECT_EQ(graph.target(backward), 0U);
    EXPECT_EQ(graph.weight(forward), 1U);
    EXPECT_EQ(graph.first_arc(1U), backward);
    EXPECT_EQ(graph.target(graph.next_arc(backward)), 2U);
    EXPECT_EQ(graph.next_arc(graph.next_arc(backward)), graph.npos);
    EXPECT_EQ(graph.first_arc(3U), graph.npos);
    EXPECT_EQ(graph.find_arc(5U, 0U), graph.npos);

    directed_graph<3U, 3U> directed(3U, {{0U, 1U}, {1U, 2U}});
    EXPECT_EQ(directed.edge_index(directed.find_arc(1U, 2U)), 1U);
}

TEST(GraphTest, RemoveEdgeUnlinksBothEndsAndRecyclesSlots)
{
    undirected_graph<5U, 4U> graph(5U, {{0U, 1U}, {0U, 2U}, {0U, 3U}, {3U, 4U}});
    EXPECT_TRUE(graph.full());

    EXPECT_EQ(graph.remove_edge(5U, 0U), status::out_of_range);
    EXPECT_EQ(graph.remove_edge(0U, 5U), status::out_of_range);
    EXPECT_EQ(graph.remove_edge(1U, 2U), status::not_found);

    // Middle of 0's list, removed through the reverse orientation.
    EXPECT_EQ(graph.remove_edge(2U, 0U), status::ok);
    EXPECT_FALSE(graph.has_edge(0U, 2U));
    EXPECT_FALSE(graph.has_edge(2U, 0U));
    EXPECT_EQ(graph.edge_count(), 3U);
    EXPECT_EQ(graph.degree(0U), 2U);
    EXPECT_EQ(graph.remove_edge(0U, 2U), status::not_found);

    // Head of 0's list.
    EXPECT_EQ(graph.remove_edge(0U, 1U), status::ok);
    EXPECT_EQ(graph.target(graph.first_arc(0U)), 3U);
    EXPECT_EQ(graph.degree(1U), 0U);

    // Freed slots are reused: two more edges fit, a third does not.
    EXPECT_EQ(graph.add_edge(1U, 4U), status::ok);
    EXPECT_EQ(graph.add_edge(2U, 4U), status::ok);
    EXPECT_EQ(graph.add_edge(1U, 2U), status::full);
    EXPECT_TRUE(graph.has_edge(4U, 1U));
    EXPECT_TRUE(graph.has_edge(4U, 2U));

    // 4's list is {3, 1, 2}: removing its tail keeps the rest in order.
    EXPECT_EQ(graph.remove_edge(2U, 4U), status::ok);
    size_type order[2] = {};
    size_type count = 0U;
    for (auto adjacent : graph.neighbors(4U))
    {
        order[count] = adjacent.node;
        ++count;
    }
    ASSERT_EQ(count, 2U);
    EXPECT_EQ(order[0], 3U);
    EXPECT_EQ(order[1], 1U);

    directed_graph<3U, 2U> directed(3U, {{0U, 1U}, {1U, 0U}});
    EXPECT_EQ(directed.remove_edge(0U, 1U), status::ok);
    EXPECT_TRUE(directed.has_edge(1U, 0U));
    EXPECT_EQ(directed.add_edge(0U, 2U), status::ok);
    EXPECT_EQ(directed.add_edge(2U, 1U), status::full);
}

TEST(GraphTest, SelfLoops)
{
    undirected_graph<2U, 2U> graph;
    EXPECT_EQ(graph.add_edge(0U, 0U), status::ok);
    EXPECT_EQ(graph.add_edge(0U, 0U), status::already_exists);
    EXPECT_TRUE(graph.has_edge(0U, 0U));
    EXPECT_EQ(graph.degree(0U), 2U);
    EXPECT_EQ(graph.add_edge(0U, 1U), status::ok);
    EXPECT_EQ(graph.remove_edge(0U, 0U), status::ok);
    EXPECT_FALSE(graph.has_edge(0U, 0U));
    EXPECT_EQ(graph.degree(0U), 1U);
    EXPECT_TRUE(graph.has_edge(1U, 0U));

    directed_graph<2U, 2U> directed;
    EXPECT_EQ(directed.add_edge(1U, 1U), status::ok);
    EXPECT_EQ(directed.degree(1U), 1U);
    EXPECT_EQ(directed.remove_edge(1U, 1U), status::ok);
    EXPECT_EQ(directed.edge_count(), 0U);
}

// ---------------------------------------------------------------------------
// Traversal and connectivity
// ---------------------------------------------------------------------------

TEST(GraphAlgorithmsTest, BfsVisitsLevelByLevel)
{
    //   0 - 1 - 3
    //   |   |
    //   2 - 4     5
    using graph_type = undirected_graph<6U, 8U>;
    const graph_type graph(6U, {{0U, 1U}, {0U, 2U}, {1U, 3U}, {1U, 4U}, {2U, 4U}});

    ga::node_list<graph_type> order;
    EXPECT_EQ(ga::bfs(graph, 0U, [&order](size_type node) { order.push_back(static_cast<uint8_t>(node)); }), 5U);
    ASSERT_EQ(order.size(), 5U);
    const size_type expected[] = {0U, 1U, 2U, 3U, 4U};
    for (size_type i = 0U; i < 5U; ++i)
    {
        EXPECT_EQ(order[i], expected[i]);
    }

    size_type calls = 0U;
    EXPECT_EQ(ga::bfs(graph, 0U, [&calls](size_type node) {
                  ++calls;
                  return node != 2U;
              }),
              3U);
    EXPECT_EQ(calls, 3U);

    EXPECT_EQ(ga::bfs(graph, 5U, [](size_type) {}), 1U);
    EXPECT_EQ(ga::bfs(graph, 6U, [](size_type) {}), 0U);
}

TEST(GraphAlgorithmsTest, DfsMatchesRecursivePreorder)
{
    //   0 -> 1 -> 3
    //   |    |
    //   v    v
    //   2 <- 4      (directed)
    using graph_type = directed_graph<5U, 6U>;
    const graph_type graph(5U, {{0U, 1U}, {0U, 2U}, {1U, 3U}, {1U, 4U}, {4U, 2U}, {3U, 0U}});

    ga::node_list<graph_type> order;
    EXPECT_EQ(ga::dfs(graph, 0U, [&order](size_type node) { order.push_back(static_cast<uint8_t>(node)); }), 5U);
    ASSERT_EQ(order.size(), 5U);
    const size_type expected[] = {0U, 1U, 3U, 4U, 2U};
    for (size_type i = 0U; i < 5U; ++i)
    {
        EXPECT_EQ(order[i], expected[i]);
    }

    EXPECT_EQ(ga::dfs(graph, 0U, [](size_type node) { return node != 3U; }), 3U);
    EXPECT_EQ(ga::dfs(graph, 0U, [](size_type) { return false; }), 1U);
    EXPECT_EQ(ga::dfs(graph, 2U, [](size_type) {}), 1U);
    EXPECT_EQ(ga::dfs(graph, 5U, [](size_type) {}), 0U);
}

TEST(GraphAlgorithmsTest, ReachableAndConnected)
{
    directed_graph<5U, 5U> directed(5U, {{0U, 1U}, {1U, 2U}, {2U, 0U}, {2U, 3U}});
    EXPECT_TRUE(ga::reachable(directed, 0U, 3U));
    EXPECT_FALSE(ga::reachable(directed, 3U, 0U));
    EXPECT_TRUE(ga::reachable(directed, 4U, 4U));
    EXPECT_FALSE(ga::reachable(directed, 0U, 4U));
    EXPECT_FALSE(ga::reachable(directed, 0U, 7U));
    EXPECT_FALSE(ga::reachable(directed, 7U, 0U));
    EXPECT_TRUE(ga::connected(directed, 0U, 2U));
    EXPECT_FALSE(ga::connected(directed, 0U, 3U));
    EXPECT_FALSE(ga::connected(directed, 3U, 0U));

    undirected_graph<5U, 5U> undirected(5U, {{0U, 1U}, {2U, 3U}, {3U, 4U}});
    EXPECT_TRUE(ga::connected(undirected, 4U, 2U));
    EXPECT_FALSE(ga::connected(undirected, 0U, 2U));
    EXPECT_TRUE(ga::reachable(undirected, 1U, 0U));
}

TEST(GraphAlgorithmsTest, ConnectedComponentsUseUnionFind)
{
    using graph_type = undirected_graph<7U, 6U>;
    const graph_type graph(7U, {{0U, 1U}, {1U, 2U}, {3U, 4U}, {6U, 6U}});
    ga::component_set<graph_type> components = ga::connected_components(graph);
    EXPECT_EQ(components.group_count(), 4U);
    EXPECT_TRUE(components.same_set(0U, 2U));
    EXPECT_FALSE(components.same_set(2U, 3U));
    EXPECT_EQ(components.group_size(4U), 2U);
    EXPECT_EQ(components.group_size(5U), 1U);

    // Directed graphs yield weakly connected components.
    directed_graph<4U, 3U> directed(4U, {{0U, 1U}, {2U, 1U}});
    EXPECT_EQ(ga::connected_components(directed).group_count(), 2U);

    undirected_graph<3U, 1U> empty_graph(0U);
    EXPECT_EQ(ga::connected_components(empty_graph).group_count(), 0U);
}

TEST(GraphAlgorithmsTest, StronglyConnectedComponentsTarjan)
{
    // {0,1,2} cycle -> {3,4} cycle -> 5 ; 6 alone with a self-loop; 7 -> 6.
    using graph_type = directed_graph<8U, 12U>;
    const graph_type graph(8U, {{0U, 1U}, {1U, 2U}, {2U, 0U}, {2U, 3U}, {3U, 4U}, {4U, 3U},
                                {4U, 5U}, {1U, 5U}, {6U, 6U}, {7U, 6U}, {5U, 4U}});
    ga::component_set<graph_type> components = ga::strongly_connected_components(graph);
    EXPECT_EQ(components.group_count(), 4U);
    EXPECT_TRUE(components.same_set(0U, 2U));
    EXPECT_TRUE(components.same_set(3U, 5U));
    EXPECT_FALSE(components.same_set(2U, 3U));
    EXPECT_FALSE(components.same_set(6U, 7U));
    EXPECT_EQ(components.group_size(0U), 3U);
    EXPECT_EQ(components.group_size(4U), 3U);

    directed_graph<4U, 3U> chain(4U, {{0U, 1U}, {1U, 2U}, {2U, 3U}});
    EXPECT_EQ(ga::strongly_connected_components(chain).group_count(), 4U);
}

// ---------------------------------------------------------------------------
// Cycles and ordering
// ---------------------------------------------------------------------------

TEST(GraphAlgorithmsTest, HasCycleUndirected)
{
    undirected_graph<5U, 6U> forest(5U, {{0U, 1U}, {1U, 2U}, {3U, 4U}});
    EXPECT_FALSE(ga::has_cycle(forest));
    EXPECT_EQ(forest.add_edge(2U, 0U), status::ok);
    EXPECT_TRUE(ga::has_cycle(forest));

    undirected_graph<2U, 2U> loop(2U, {{0U, 1U}, {1U, 1U}});
    EXPECT_TRUE(ga::has_cycle(loop));
}

TEST(GraphAlgorithmsTest, HasCycleDirected)
{
    directed_graph<4U, 5U> dag(4U, {{0U, 1U}, {0U, 2U}, {1U, 3U}, {2U, 3U}});
    EXPECT_FALSE(ga::has_cycle(dag));
    EXPECT_EQ(dag.add_edge(3U, 0U), status::ok);
    EXPECT_TRUE(ga::has_cycle(dag));

    directed_weighted_graph<2U, 1U> loop(2U, {{1U, 1U, 3}});
    EXPECT_TRUE(ga::has_cycle(loop));
}

TEST(GraphAlgorithmsTest, TopologicalSortKahn)
{
    // Boot dependencies: 5 -> 2 -> 3 -> 1, 5 -> 0, 4 -> 0, 4 -> 1.
    using graph_type = directed_graph<6U, 8U>;
    graph_type graph(6U, {{5U, 2U}, {5U, 0U}, {4U, 0U}, {4U, 1U}, {2U, 3U}, {3U, 1U}});
    ga::node_list<graph_type> order;
    ASSERT_EQ(ga::topological_sort(graph, order), status::ok);
    ASSERT_EQ(order.size(), 6U);
    const size_type expected[] = {4U, 5U, 2U, 0U, 3U, 1U};
    for (size_type i = 0U; i < 6U; ++i)
    {
        EXPECT_EQ(order[i], expected[i]);
    }
    for (size_type node = 0U; node < graph.size(); ++node)
    {
        for (auto adjacent : graph.neighbors(node))
        {
            EXPECT_LT(position_of<graph_type>(order, node), position_of<graph_type>(order, adjacent.node));
        }
    }

    // 1 -> 2 -> 3 -> 1 is a cycle; only 0, 4 and 5 can be ordered.
    graph_type cyclic(6U, {{0U, 1U}, {1U, 2U}, {2U, 3U}, {3U, 1U}, {4U, 5U}});
    EXPECT_EQ(ga::topological_sort(cyclic, order), status::invalid_argument);
    ASSERT_EQ(order.size(), 3U);
    EXPECT_EQ(order[0], 0U);
    EXPECT_EQ(order[1], 4U);
    EXPECT_EQ(order[2], 5U);
}

// ---------------------------------------------------------------------------
// Shortest paths
// ---------------------------------------------------------------------------

TEST(GraphAlgorithmsTest, ShortestPathsUnweightedUseHopCount)
{
    using graph_type = undirected_graph<6U, 7U>;
    const graph_type graph(6U, {{0U, 1U}, {1U, 2U}, {2U, 3U}, {0U, 4U}, {4U, 3U}});
    ga::shortest_path_tree<graph_type> tree;
    EXPECT_TRUE(tree.empty());
    EXPECT_EQ(tree.source(), tree.npos);

    ASSERT_EQ(ga::shortest_paths(graph, 0U, tree), status::ok);
    EXPECT_EQ(tree.size(), 6U);
    EXPECT_EQ(tree.source(), 0U);
    EXPECT_EQ(tree.distance(0U), 0U);
    EXPECT_EQ(tree.distance(2U), 2U);
    EXPECT_EQ(tree.distance(3U), 2U);
    EXPECT_EQ(tree.parent(3U), 4U);
    EXPECT_EQ(tree.parent(0U), tree.npos);
    EXPECT_FALSE(tree.reachable(5U));
    EXPECT_EQ(tree.distance(5U), ga::infinity<graph_type>);
    EXPECT_EQ(tree.parent(5U), tree.npos);
    EXPECT_EQ(tree.distance(9U), ga::infinity<graph_type>);
    EXPECT_FALSE(tree.reachable(9U));
    EXPECT_EQ(tree.parent(9U), tree.npos);

    ga::node_list<graph_type> path;
    ASSERT_EQ(tree.path_to(3U, path), status::ok);
    ASSERT_EQ(path.size(), 3U);
    EXPECT_EQ(path[0], 0U);
    EXPECT_EQ(path[1], 4U);
    EXPECT_EQ(path[2], 3U);
    ASSERT_EQ(tree.path_to(0U, path), status::ok);
    ASSERT_EQ(path.size(), 1U);
    EXPECT_EQ(tree.path_to(5U, path), status::not_found);
    EXPECT_TRUE(path.empty());
    EXPECT_EQ(tree.path_to(6U, path), status::out_of_range);

    EXPECT_EQ(ga::shortest_paths(graph, 6U, tree), status::out_of_range);
    EXPECT_TRUE(tree.empty());
    EXPECT_FALSE(tree.reachable(0U));

    // Point queries stop the BFS once the target is dequeued.
    EXPECT_EQ(ga::distance(graph, 1U, 3U), 2U);
    EXPECT_EQ(ga::distance(graph, 1U, 5U), ga::infinity<graph_type>);
    ASSERT_EQ(ga::shortest_path(graph, 2U, 4U, path), status::ok);
    ASSERT_EQ(path.size(), 3U);
    EXPECT_EQ(path[1], 3U);
}

TEST(GraphAlgorithmsTest, DijkstraOnWeightedDirectedGraph)
{
    using graph_type = directed_weighted_graph<6U, 10U, uint32_t>;
    const graph_type graph(6U, {{0U, 1U, 7U}, {0U, 2U, 9U}, {0U, 5U, 14U}, {1U, 2U, 10U}, {1U, 3U, 15U},
                                {2U, 3U, 11U}, {2U, 5U, 2U}, {3U, 4U, 6U}, {5U, 4U, 9U}});
    ga::shortest_path_tree<graph_type> tree;
    ASSERT_EQ(ga::shortest_paths(graph, 0U, tree), status::ok);
    const uint32_t expected[] = {0U, 7U, 9U, 20U, 20U, 11U};
    for (size_type node = 0U; node < 6U; ++node)
    {
        EXPECT_EQ(tree.distance(node), expected[node]);
    }
    EXPECT_EQ(tree.parent(5U), 2U);
    EXPECT_EQ(tree.parent(4U), 5U);
    EXPECT_TRUE(tree.reachable(4U));
    ASSERT_EQ(ga::shortest_paths(graph, 3U, tree), status::ok);
    EXPECT_FALSE(tree.reachable(0U));
    EXPECT_TRUE(tree.reachable(4U));

    ga::node_list<graph_type> path;
    ASSERT_EQ(ga::shortest_path(graph, 0U, 4U, path), status::ok);
    ASSERT_EQ(path.size(), 4U);
    EXPECT_EQ(path[0], 0U);
    EXPECT_EQ(path[1], 2U);
    EXPECT_EQ(path[2], 5U);
    EXPECT_EQ(path[3], 4U);

    EXPECT_EQ(ga::distance(graph, 0U, 3U), 20U);
    EXPECT_EQ(ga::distance(graph, 4U, 0U), ga::infinity<graph_type>);
    EXPECT_EQ(ga::shortest_path(graph, 4U, 0U, path), status::not_found);
    EXPECT_EQ(ga::shortest_path(graph, 6U, 0U, path), status::out_of_range);
    EXPECT_EQ(ga::shortest_path(graph, 0U, 6U, path), status::out_of_range);
    EXPECT_TRUE(path.empty());
}

TEST(GraphAlgorithmsTest, DijkstraRejectsNegativeWeights)
{
    using graph_type = undirected_weighted_graph<3U, 3U, int16_t>;
    const graph_type graph(3U, {{0U, 1U, 4}, {1U, 2U, -1}});
    ga::shortest_path_tree<graph_type> tree;
    EXPECT_EQ(ga::shortest_paths(graph, 0U, tree), status::invalid_argument);
    EXPECT_TRUE(tree.empty());
    EXPECT_EQ(ga::distance(graph, 0U, 2U), ga::infinity<graph_type>);
    ga::node_list<graph_type> path;
    EXPECT_EQ(ga::shortest_path(graph, 0U, 2U, path), status::invalid_argument);
    // The search stops at the target before it scans the negative arc.
    EXPECT_EQ(ga::distance(graph, 0U, 1U), 4);
}

TEST(GraphAlgorithmsTest, DijkstraAgreesWithFloydWarshallOnDenseGraph)
{
    // Deterministic pseudo-random dense graph: exercises every heap sift branch.
    using graph_type = undirected_weighted_graph<12U, 66U, uint16_t>;
    graph_type graph;
    uint32_t seed = 12345U;
    for (size_type from = 0U; from < 12U; ++from)
    {
        for (size_type to = from + 1U; to < 12U; ++to)
        {
            seed = seed * 1103515245U + 12345U;
            if ((seed >> 16U) % 3U != 0U)
            {
                EXPECT_EQ(graph.add_edge(from, to, static_cast<uint16_t>(1U + (seed >> 20U) % 50U)), status::ok);
            }
        }
    }

    ga::distance_matrix<graph_type> matrix;
    ASSERT_EQ(ga::floyd_warshall(graph, matrix), status::ok);
    ga::shortest_path_tree<graph_type> dijkstra_tree;
    ga::shortest_path_tree<graph_type> bellman_tree;
    for (size_type source = 0U; source < 12U; ++source)
    {
        ASSERT_EQ(ga::shortest_paths(graph, source, dijkstra_tree), status::ok);
        ASSERT_EQ(ga::bellman_ford(graph, source, bellman_tree), status::ok);
        for (size_type node = 0U; node < 12U; ++node)
        {
            EXPECT_EQ(dijkstra_tree.distance(node), matrix(source, node));
            EXPECT_EQ(bellman_tree.distance(node), matrix(source, node));
        }
    }
}

TEST(GraphAlgorithmsTest, DijkstraWithFloatWeights)
{
    using graph_type = directed_weighted_graph<4U, 5U, float>;
    const graph_type graph(4U, {{0U, 1U, 0.5F}, {1U, 2U, 0.25F}, {0U, 2U, 1.0F}, {2U, 3U, 2.0F}});
    EXPECT_FLOAT_EQ(ga::distance(graph, 0U, 3U), 2.75F);
}

TEST(GraphAlgorithmsTest, BellmanFordHandlesNegativeWeights)
{
    using graph_type = directed_weighted_graph<5U, 9U>;
    graph_type graph(5U, {{0U, 1U, 6}, {0U, 2U, 7}, {1U, 2U, 8}, {1U, 3U, 5}, {1U, 4U, -4},
                          {2U, 3U, -3}, {2U, 4U, 9}, {3U, 1U, -2}});
    ga::shortest_path_tree<graph_type> tree;
    ASSERT_EQ(ga::bellman_ford(graph, 0U, tree), status::ok);
    const int32_t expected[] = {0, 2, 7, 4, -2};
    for (size_type node = 0U; node < 5U; ++node)
    {
        EXPECT_EQ(tree.distance(node), expected[node]);
    }
    ga::node_list<graph_type> path;
    ASSERT_EQ(tree.path_to(4U, path), status::ok);
    ASSERT_EQ(path.size(), 5U);
    EXPECT_EQ(path[1], 2U);
    EXPECT_EQ(path[2], 3U);
    EXPECT_EQ(path[3], 1U);

    // Node 4 has no outgoing arcs: every other node stays unreachable.
    ASSERT_EQ(ga::bellman_ford(graph, 4U, tree), status::ok);
    EXPECT_FALSE(tree.reachable(0U));
    EXPECT_EQ(tree.distance(4U), 0);

    EXPECT_EQ(ga::bellman_ford(graph, 5U, tree), status::out_of_range);
    EXPECT_TRUE(tree.empty());

    EXPECT_EQ(graph.add_edge(4U, 3U, 1), status::ok); // 3 -> 1 -> 4 -> 3 weighs -5
    EXPECT_EQ(ga::bellman_ford(graph, 0U, tree), status::invalid_argument);
    EXPECT_TRUE(tree.empty());
    EXPECT_EQ(tree.path_to(0U, path), status::out_of_range);

    // An undirected negative edge is a two-arc negative cycle.
    using negative_type = undirected_weighted_graph<2U, 1U>;
    const negative_type negative_edge(2U, {{0U, 1U, -1}});
    ga::shortest_path_tree<negative_type> negative_tree;
    EXPECT_EQ(ga::bellman_ford(negative_edge, 0U, negative_tree), status::invalid_argument);

    // A single node with a negative self-loop is detected in the only round.
    directed_weighted_graph<1U, 1U> self_loop(1U, {{0U, 0U, -1}});
    ga::shortest_path_tree<directed_weighted_graph<1U, 1U>> self_tree;
    EXPECT_EQ(ga::bellman_ford(self_loop, 0U, self_tree), status::invalid_argument);

    // Unweighted graphs relax with unit weights.
    using hops_type = directed_graph<3U, 2U>;
    const hops_type hops(3U, {{0U, 1U}, {1U, 2U}});
    ga::shortest_path_tree<hops_type> hop_tree;
    ASSERT_EQ(ga::bellman_ford(hops, 0U, hop_tree), status::ok);
    EXPECT_EQ(hop_tree.distance(2U), 2U);
}

TEST(GraphAlgorithmsTest, FloydWarshallAllPairs)
{
    using graph_type = directed_weighted_graph<4U, 6U>;
    const graph_type graph(3U, {{0U, 1U, 5}, {1U, 2U, -2}, {0U, 2U, 4}, {2U, 2U, 3}});
    ga::distance_matrix<graph_type> matrix;
    matrix.fill(77);
    ASSERT_EQ(ga::floyd_warshall(graph, matrix), status::ok);
    EXPECT_EQ(matrix(0U, 0U), 0);
    EXPECT_EQ(matrix(0U, 1U), 5);
    EXPECT_EQ(matrix(0U, 2U), 3);
    EXPECT_EQ(matrix(1U, 2U), -2);
    EXPECT_EQ(matrix(2U, 2U), 0);
    EXPECT_EQ(matrix(1U, 0U), ga::infinity<graph_type>);
    EXPECT_EQ(matrix(2U, 0U), ga::infinity<graph_type>);
    EXPECT_EQ(matrix(3U, 3U), 77);
    EXPECT_EQ(matrix(0U, 3U), 77);

    const graph_type negative_loop(2U, {{0U, 1U, 1}, {1U, 1U, -1}});
    EXPECT_EQ(ga::floyd_warshall(negative_loop, matrix), status::invalid_argument);

    const graph_type negative_cycle(3U, {{0U, 1U, 1}, {1U, 2U, -3}, {2U, 0U, 1}});
    EXPECT_EQ(ga::floyd_warshall(negative_cycle, matrix), status::invalid_argument);

    using hops_type = undirected_graph<4U, 3U>;
    const hops_type hops(4U, {{0U, 1U}, {1U, 2U}});
    ga::distance_matrix<hops_type> hop_matrix;
    ASSERT_EQ(ga::floyd_warshall(hops, hop_matrix), status::ok);
    EXPECT_EQ(hop_matrix(2U, 0U), 2U);
    EXPECT_EQ(hop_matrix(0U, 3U), ga::infinity<hops_type>);
}

// ---------------------------------------------------------------------------
// Spanning trees, bridges, articulation points
// ---------------------------------------------------------------------------

TEST(GraphAlgorithmsTest, MinimumSpanningTreeKruskal)
{
    using graph_type = undirected_weighted_graph<9U, 14U>;
    const graph_type graph(9U, {{0U, 1U, 4}, {0U, 7U, 8}, {1U, 2U, 8}, {1U, 7U, 11}, {2U, 3U, 7},
                                {2U, 8U, 2}, {2U, 5U, 4}, {3U, 4U, 9}, {3U, 5U, 14}, {4U, 5U, 10},
                                {5U, 6U, 2}, {6U, 7U, 1}, {6U, 8U, 6}, {7U, 8U, 7}});
    ga::edge_list<graph_type> tree;
    EXPECT_EQ(ga::minimum_spanning_tree(graph, tree), 37);
    ASSERT_EQ(tree.size(), 8U);
    EXPECT_EQ(tree[0].from, 6U);
    EXPECT_EQ(tree[0].to, 7U);
    EXPECT_EQ(tree[0].weight, 1);
    EXPECT_EQ(tree[1].from, 2U); // equal weights keep edge slot order
    EXPECT_EQ(tree[1].to, 8U);
    EXPECT_EQ(tree[2].from, 5U);
    EXPECT_EQ(tree[2].to, 6U);
    for (size_type i = 1U; i < tree.size(); ++i)
    {
        EXPECT_LE(tree[i - 1U].weight, tree[i].weight);
    }

    using forest_type = undirected_graph<6U, 6U>;
    const forest_type forest(6U, {{0U, 1U}, {1U, 2U}, {2U, 0U}, {3U, 4U}, {4U, 4U}});
    ga::edge_list<forest_type> spanning_forest;
    EXPECT_EQ(ga::minimum_spanning_tree(forest, spanning_forest), 3U);
    ASSERT_EQ(spanning_forest.size(), 3U);
    EXPECT_EQ(spanning_forest[2].from, 3U);
    EXPECT_EQ(spanning_forest[2].to, 4U);

    const forest_type empty_graph(0U);
    EXPECT_EQ(ga::minimum_spanning_tree(empty_graph, spanning_forest), 0U);
    EXPECT_TRUE(spanning_forest.empty());

    // Unweighted: edges are taken in scan order, so node 0 spans K4.
    const forest_type complete(4U, {{0U, 1U}, {0U, 2U}, {0U, 3U}, {1U, 2U}, {1U, 3U}, {2U, 3U}});
    EXPECT_EQ(ga::minimum_spanning_tree(complete, spanning_forest), 3U);
    ASSERT_EQ(spanning_forest.size(), 3U);
    EXPECT_EQ(spanning_forest[0].to, 1U);
    EXPECT_EQ(spanning_forest[1].to, 2U);
    EXPECT_EQ(spanning_forest[2].to, 3U);
}

TEST(GraphAlgorithmsTest, BridgesAndArticulationPoints)
{
    // Triangle {0,1,2} - bridge 1-3 - triangle {3,4,5} - bridge 5-6.
    using graph_type = undirected_graph<7U, 8U>;
    const graph_type graph(7U, {{0U, 1U}, {1U, 2U}, {2U, 0U}, {1U, 3U}, {3U, 4U}, {4U, 5U}, {5U, 3U}, {5U, 6U}});
    ga::edge_list<graph_type> bridge_list;
    ga::bridges(graph, bridge_list);
    ASSERT_EQ(bridge_list.size(), 2U);
    EXPECT_EQ(bridge_list[0].from, 5U);
    EXPECT_EQ(bridge_list[0].to, 6U);
    EXPECT_EQ(bridge_list[1].from, 1U);
    EXPECT_EQ(bridge_list[1].to, 3U);

    ga::node_list<graph_type> cut_nodes;
    ga::articulation_points(graph, cut_nodes);
    ASSERT_EQ(cut_nodes.size(), 3U);
    EXPECT_EQ(cut_nodes[0], 1U);
    EXPECT_EQ(cut_nodes[1], 3U);
    EXPECT_EQ(cut_nodes[2], 5U);

    // A DFS root with two children is a cut node; a self-loop never is a bridge.
    const graph_type star(5U, {{0U, 1U}, {0U, 2U}, {2U, 2U}, {3U, 4U}});
    ga::bridges(star, bridge_list);
    ASSERT_EQ(bridge_list.size(), 3U);
    ga::articulation_points(star, cut_nodes);
    ASSERT_EQ(cut_nodes.size(), 1U);
    EXPECT_EQ(cut_nodes[0], 0U);

    // A path rooted at an end: only the inner node is a cut node.
    const graph_type path(3U, {{0U, 1U}, {1U, 2U}});
    ga::articulation_points(path, cut_nodes);
    ASSERT_EQ(cut_nodes.size(), 1U);
    EXPECT_EQ(cut_nodes[0], 1U);

    const graph_type cycle(4U, {{0U, 1U}, {1U, 2U}, {2U, 3U}, {3U, 0U}});
    ga::bridges(cycle, bridge_list);
    EXPECT_TRUE(bridge_list.empty());
    ga::articulation_points(cycle, cut_nodes);
    EXPECT_TRUE(cut_nodes.empty());

    using weighted_type = undirected_weighted_graph<3U, 2U, uint8_t>;
    const weighted_type weighted(3U, {{0U, 1U, 3U}, {1U, 2U, 9U}});
    ga::edge_list<weighted_type> weighted_bridges;
    ga::bridges(weighted, weighted_bridges);
    ASSERT_EQ(weighted_bridges.size(), 2U);
    EXPECT_EQ(weighted_bridges[0].weight, 9U);
    EXPECT_EQ(weighted_bridges[1].weight, 3U);
}

// ---------------------------------------------------------------------------
// Coloring
// ---------------------------------------------------------------------------

TEST(GraphAlgorithmsTest, GreedyColoringWelshPowell)
{
    using graph_type = undirected_graph<6U, 10U>;
    ga::color_map<graph_type> colors;

    const graph_type complete(4U, {{0U, 1U}, {0U, 2U}, {0U, 3U}, {1U, 2U}, {1U, 3U}, {2U, 3U}});
    EXPECT_EQ(ga::greedy_coloring(complete, colors), 4U);
    EXPECT_TRUE(is_proper_coloring(complete, colors, 4U));

    // Hub 3 has the highest degree, so it is colored first with color 0.
    const graph_type wheel(6U, {{3U, 0U}, {3U, 1U}, {3U, 2U}, {3U, 4U}, {3U, 5U},
                                {0U, 1U}, {1U, 2U}, {2U, 4U}, {4U, 5U}, {5U, 0U}});
    const size_type used = ga::greedy_coloring(wheel, colors);
    EXPECT_EQ(colors[3], 0U);
    EXPECT_LE(used, 4U);
    EXPECT_TRUE(is_proper_coloring(wheel, colors, used));

    // Self-loops are ignored; isolated nodes take color 0.
    const graph_type loop(3U, {{0U, 0U}, {0U, 1U}});
    EXPECT_EQ(ga::greedy_coloring(loop, colors), 2U);
    EXPECT_TRUE(is_proper_coloring(loop, colors, 2U));
    EXPECT_EQ(colors[2], 0U);

    const graph_type empty_graph(0U);
    EXPECT_EQ(ga::greedy_coloring(empty_graph, colors), 0U);
    EXPECT_TRUE(colors.empty());
}

TEST(GraphAlgorithmsTest, BipartiteColoring)
{
    using graph_type = undirected_graph<7U, 8U>;
    ga::color_map<graph_type> colors;

    const graph_type even_cycle(7U, {{0U, 1U}, {1U, 2U}, {2U, 3U}, {3U, 0U}, {5U, 6U}});
    ASSERT_TRUE(ga::bipartite_coloring(even_cycle, colors));
    ASSERT_EQ(colors.size(), 7U);
    EXPECT_EQ(colors[0], 0U);
    EXPECT_EQ(colors[1], 1U);
    EXPECT_EQ(colors[2], 0U);
    EXPECT_EQ(colors[3], 1U);
    EXPECT_EQ(colors[4], 0U);
    EXPECT_EQ(colors[5], 0U);
    EXPECT_EQ(colors[6], 1U);
    EXPECT_TRUE(is_proper_coloring(even_cycle, colors, 2U));

    const graph_type odd_cycle(5U, {{0U, 1U}, {1U, 2U}, {2U, 0U}});
    EXPECT_FALSE(ga::bipartite_coloring(odd_cycle, colors));

    const graph_type loop(2U, {{0U, 1U}, {1U, 1U}});
    EXPECT_FALSE(ga::bipartite_coloring(loop, colors));
}

} // namespace
