#include "sample_support.hpp"

#include "castle/dsa/grid.hpp"

namespace
{
enum class tile : uint8_t
{
    free,
    wall,
    robot
};

// Calibration map indexed by (rpm band, load band); values are injector pulse widths in microseconds.
using injection_map = castle::dsa::grid<uint16_t, 3U, 4U>;

constexpr injection_map k_injection_us(
    900U, 1100U, 1300U, 1500U,
    1000U, 1250U, 1500U, 1750U,
    1100U, 1400U, 1700U, 2000U);

static_assert(injection_map::static_rows == 3U, "rpm bands");
static_assert(injection_map::static_columns == 4U, "load bands");
static_assert(sizeof(injection_map) == sizeof(uint16_t) * 12U, "no storage overhead");
static_assert(k_injection_us(2U, 3U) == 2000U, "constexpr lookup");
} // namespace

int main()
{
    CASTLE_SAMPLE_CHECK(k_injection_us.rows() == 3U);
    CASTLE_SAMPLE_CHECK(k_injection_us.columns() == 4U);
    CASTLE_SAMPLE_CHECK(k_injection_us.size() == 12U);
    CASTLE_SAMPLE_CHECK(k_injection_us.at(1U, 2U) == 1500U);

    // Each row is contiguous, so it can be handed out as an array_view.
    castle::dsa::array_view<const uint16_t> idle_band = k_injection_us.row(0U);
    CASTLE_SAMPLE_CHECK(idle_band.size() == 4U);
    CASTLE_SAMPLE_CHECK(idle_band.front() == 900U);
    CASTLE_SAMPLE_CHECK(idle_band.back() == 1500U);

    uint32_t total_us = 0U;
    for (uint16_t pulse : k_injection_us)
    {
        total_us += pulse;
    }
    CASTLE_SAMPLE_CHECK(total_us == 16500U);

    // Non-arithmetic element types work too: a small occupancy map.
    castle::dsa::grid<tile, 2U, 3U> floor;
    CASTLE_SAMPLE_CHECK(floor(1U, 2U) == tile::free);
    floor.fill(tile::free);
    floor(0U, 1U) = tile::wall;
    floor.at(1U, 0U) = tile::robot;

    castle::dsa::array_view<tile> bottom = floor.row(1U);
    bottom[2U] = tile::wall;
    CASTLE_SAMPLE_CHECK(floor(1U, 2U) == tile::wall);

    const castle::dsa::grid<tile, 2U, 3U> expected{
        tile::free, tile::wall, tile::free,
        tile::robot, tile::free, tile::wall};
    CASTLE_SAMPLE_CHECK(floor == expected);

    floor(1U, 0U) = tile::free;
    CASTLE_SAMPLE_CHECK(floor != expected);
    CASTLE_SAMPLE_CHECK(*floor.rbegin() == tile::wall);

    return 0;
}
