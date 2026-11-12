#include "sample_support.hpp"

#include "castle/dsa/disjoint_set.hpp"

namespace
{
struct harness_link
{
    uint8_t from;
    uint8_t to;
    uint16_t cost; // cable length in cm
};

constexpr castle::size_type k_ecus = 6U;

// Candidate harness segments between six ECUs, sorted by ascending cost for Kruskal.
constexpr harness_link k_links[] = {
    {0U, 1U, 40U},
    {1U, 2U, 55U},
    {0U, 2U, 60U}, // closes a cycle with the two links above
    {3U, 4U, 70U},
    {2U, 3U, 90U},
    {4U, 5U, 95U},
    {1U, 5U, 120U},
};

using ecu_set = castle::dsa::disjoint_set<k_ecus>;
} // namespace

int main()
{
    // Kruskal: take each link in cost order unless both ends are already connected.
    ecu_set network;
    uint32_t harness_cm = 0U;
    castle::size_type used_links = 0U;
    for (const harness_link& candidate : k_links)
    {
        if (network.unite(candidate.from, candidate.to))
        {
            harness_cm += candidate.cost;
            ++used_links;
        }
    }
    CASTLE_SAMPLE_CHECK(used_links == k_ecus - 1U);
    CASTLE_SAMPLE_CHECK(harness_cm == 40U + 55U + 70U + 90U + 95U);
    CASTLE_SAMPLE_CHECK(network.group_count() == 1U);
    CASTLE_SAMPLE_CHECK(network.same_set(0U, 5U));

    // Islands of a partially wired bench setup: only the first and fourth links are installed.
    ecu_set bench;
    bench.unite(k_links[0].from, k_links[0].to);
    bench.unite(k_links[3].from, k_links[3].to);
    CASTLE_SAMPLE_CHECK(bench.group_count() == 4U);
    CASTLE_SAMPLE_CHECK(bench.group_size(1U) == 2U);

    // Islands in order of their lowest ECU id: {0,1} {2} {3,4} {5}.
    const ecu_set::group_list islands = bench.groups();
    CASTLE_SAMPLE_CHECK(islands.size() == 4U);
    CASTLE_SAMPLE_CHECK(islands.group_of(4U) == 2U);
    castle::size_type wired_ecus = 0U;
    for (ecu_set::group_list::value_type island : islands)
    {
        if (island.size() > 1U)
        {
            wired_ecus += island.size();
        }
    }
    CASTLE_SAMPLE_CHECK(wired_ecus == 4U);
    CASTLE_SAMPLE_CHECK(islands[2U][0U] == 3U && islands[2U][1U] == 4U);

    // ECU 4 is pulled for replacement: it becomes its own island again.
    bench.reset(4U);
    CASTLE_SAMPLE_CHECK(bench.group_count() == 5U);
    CASTLE_SAMPLE_CHECK(!bench.same_set(3U, 4U));
    CASTLE_SAMPLE_CHECK(bench.group_size(3U) == 1U);

    // Tear the bench down; a smaller rig reuses the same object.
    bench.reset();
    CASTLE_SAMPLE_CHECK(bench.group_count() == k_ecus);
    CASTLE_SAMPLE_CHECK(bench.assign(3U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(bench.size() == 3U && bench.group_count() == 3U);
    CASTLE_SAMPLE_CHECK(bench.assign(k_ecus + 1U) == castle::status::out_of_range);

    return 0;
}
