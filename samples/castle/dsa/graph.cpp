#include "sample_support.hpp"

#include "castle/dsa/graph.hpp"

namespace
{
namespace ga = castle::dsa::graph_algorithms;

// ECU harness: nodes are ECUs, edge weights are cable lengths in cm.
//
//   0 GW ---40--- 1 BCM ---55--- 2 ADAS
//      \           |              |
//       60        90             70
//         \        |              |
//          `----- 3 HU ---125--- 4 CAM ---30--- 5 RADAR
using harness_graph = castle::dsa::undirected_weighted_graph<8U, 12U, uint16_t>;

// Boot dependencies: an edge `a -> b` means `a` must be up before `b` starts.
using boot_graph = castle::dsa::directed_graph<6U, 8U>;

// Robot aisle map: directed segments weighted by energy in Wh; downhill segments recover energy.
using route_graph = castle::dsa::directed_weighted_graph<5U, 8U, int16_t>;

// Radio interference between sensor nodes: adjacent nodes must not share a channel.
using interference_graph = castle::dsa::undirected_graph<6U, 10U>;

void harness_planning()
{
    const harness_graph harness(6U, {{0U, 1U, 40U},
                                     {1U, 2U, 55U},
                                     {0U, 3U, 60U},
                                     {1U, 3U, 90U},
                                     {2U, 4U, 70U},
                                     {3U, 4U, 125U},
                                     {4U, 5U, 30U}});

    // Cheapest cable set that still links every ECU (Kruskal).
    ga::edge_list<harness_graph> wiring;
    CASTLE_SAMPLE_CHECK(ga::minimum_spanning_tree(harness, wiring) == 30U + 40U + 55U + 60U + 70U);
    CASTLE_SAMPLE_CHECK(wiring.size() == harness.size() - 1U);

    // Shortest cable run from the gateway to the radar (Dijkstra): 0-1-2-4-5.
    CASTLE_SAMPLE_CHECK(ga::distance(harness, 0U, 5U) == 40U + 55U + 70U + 30U);
    ga::node_list<harness_graph> run;
    CASTLE_SAMPLE_CHECK(ga::shortest_path(harness, 0U, 5U, run) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(run.size() == 5U && run[3] == 4U);

    // Single points of failure: the radar hangs off one cable, and the camera is its only link.
    ga::edge_list<harness_graph> fragile_cables;
    ga::bridges(harness, fragile_cables);
    CASTLE_SAMPLE_CHECK(fragile_cables.size() == 1U);
    CASTLE_SAMPLE_CHECK(fragile_cables[0].from == 4U && fragile_cables[0].to == 5U);

    ga::node_list<harness_graph> critical_ecus;
    ga::articulation_points(harness, critical_ecus);
    CASTLE_SAMPLE_CHECK(critical_ecus.size() == 1U && critical_ecus[0] == 4U);
}

void boot_sequencing()
{
    // 0 power, 1 clock, 2 flash, 3 can, 4 diag, 5 app.
    boot_graph boot(6U, {{0U, 1U}, {0U, 2U}, {1U, 3U}, {2U, 5U}, {3U, 4U}, {3U, 5U}});

    ga::node_list<boot_graph> order;
    CASTLE_SAMPLE_CHECK(ga::topological_sort(boot, order) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(order.size() == 6U && order[0] == 0U && order[5] == 5U);
    CASTLE_SAMPLE_CHECK(ga::reachable(boot, 0U, 4U));
    CASTLE_SAMPLE_CHECK(!ga::reachable(boot, 4U, 0U));

    // A misconfigured dependency closes a loop: the sequencer must refuse it.
    CASTLE_SAMPLE_CHECK(boot.add_edge(4U, 1U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(ga::has_cycle(boot));
    CASTLE_SAMPLE_CHECK(ga::topological_sort(boot, order) == castle::status::invalid_argument);

    // The loop {1, 3, 4} is one strongly connected component.
    ga::component_set<boot_graph> groups = ga::strongly_connected_components(boot);
    CASTLE_SAMPLE_CHECK(groups.group_count() == 4U && groups.same_set(1U, 4U));

    CASTLE_SAMPLE_CHECK(boot.remove_edge(4U, 1U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(!ga::has_cycle(boot));
}

void robot_routing()
{
    // 0 dock, 1 aisle A, 2 ramp top, 3 aisle B, 4 pick station.
    route_graph route(5U, {{0U, 1U, 4}, {0U, 2U, 9}, {1U, 3U, 6}, {2U, 3U, -3}, {3U, 4U, 2}, {1U, 4U, 9}});

    // The downhill ramp recovers energy, so Dijkstra refuses the graph and Bellman-Ford solves it.
    ga::shortest_path_tree<route_graph> energy;
    CASTLE_SAMPLE_CHECK(ga::shortest_paths(route, 0U, energy) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(ga::bellman_ford(route, 0U, energy) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(energy.distance(4U) == 8);
    CASTLE_SAMPLE_CHECK(energy.parent(4U) == 3U && energy.parent(3U) == 2U);

    // Precomputed energy table for every dock/station pair.
    ga::distance_matrix<route_graph> table;
    CASTLE_SAMPLE_CHECK(ga::floyd_warshall(route, table) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(table(0U, 3U) == 6);
    CASTLE_SAMPLE_CHECK(table(4U, 0U) == ga::infinity<route_graph>);
}

void channel_assignment()
{
    interference_graph radio(6U, {{0U, 1U}, {1U, 2U}, {2U, 0U}, {2U, 3U}, {4U, 5U}});

    ga::color_map<interference_graph> channel;
    CASTLE_SAMPLE_CHECK(ga::greedy_coloring(radio, channel) == 3U); // the triangle needs three channels
    CASTLE_SAMPLE_CHECK(channel[0] != channel[1] && channel[1] != channel[2] && channel[2] != channel[3]);
    CASTLE_SAMPLE_CHECK(!ga::bipartite_coloring(radio, channel));

    // Two isolated radio cells; node 3 is one hop from the triangle.
    ga::component_set<interference_graph> cells = ga::connected_components(radio);
    CASTLE_SAMPLE_CHECK(cells.group_count() == 2U);
    CASTLE_SAMPLE_CHECK(ga::distance(radio, 0U, 3U) == 2U);
    CASTLE_SAMPLE_CHECK(!ga::connected(radio, 0U, 5U));

    // Breadth-first discovery from node 0 stops as soon as node 3 is found.
    castle::size_type discovered = 0U;
    ga::bfs(radio, 0U, [&discovered](castle::size_type node) {
        ++discovered;
        return node != 3U;
    });
    CASTLE_SAMPLE_CHECK(discovered == 4U);

    // Dropping the 2-0 link breaks the triangle: two channels suffice.
    CASTLE_SAMPLE_CHECK(radio.remove_edge(0U, 2U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(ga::bipartite_coloring(radio, channel));
}
} // namespace

int main()
{
    harness_planning();
    boot_sequencing();
    robot_routing();
    channel_assignment();
    return 0;
}
