#include "sample_support.hpp"

#include "castle/dsa/hyper_log_log.hpp"

namespace
{
// Packs a position, quantized to a grid of 1e-5 degree (about 1.1 m), into one key.
uint64_t grid_cell(int32_t latitude_e7, int32_t longitude_e7)
{
    const uint32_t row = static_cast<uint32_t>(latitude_e7 / 100);
    const uint32_t column = static_cast<uint32_t>(longitude_e7 / 100);
    return (static_cast<uint64_t>(row) << 32U) | column;
}
} // namespace

int main()
{
    // 1) Vehicle stands still: how many distinct ~1 m cells does the GNSS fix jump around?
    //    3600 fixes (one hour at 1 Hz) cost 256 bytes instead of a table of every cell.
    castle::dsa::hyper_log_log<uint64_t, 8U> parked;
    const int32_t latitude_e7 = 375665000;   // 37.5665000 deg
    const int32_t longitude_e7 = 1269780000; // 126.9780000 deg
    for (int32_t second = 0; second < 3600; ++second)
    {
        // Deterministic wander over a 7 x 5 cell footprint around the true position.
        const int32_t north = ((second * 3) % 7 - 3) * 100;
        const int32_t east = ((second * 2) % 5 - 2) * 100;
        parked.add(grid_cell(latitude_e7 + north, longitude_e7 + east));
    }
    const uint64_t cells = parked.estimate();
    CASTLE_SAMPLE_CHECK(cells >= 33U && cells <= 37U); // 35 cells: small counts are nearly exact

    // 2) Distinct CAN identifiers on two buses, combined at the gateway with merge().
    castle::dsa::hyper_log_log<uint32_t> powertrain; // 1024 bytes, about 3 % standard error
    castle::dsa::hyper_log_log<uint32_t> chassis;
    for (uint32_t frame = 0U; frame < 20000U; ++frame)
    {
        powertrain.add(0x100U + (frame % 1500U)); // ids 0x100 .. 0x6DB
        chassis.add(0x600U + (frame % 1000U));    // ids 0x600 .. 0x9E7, 220 shared with powertrain
    }
    const uint64_t powertrain_ids = powertrain.estimate();
    CASTLE_SAMPLE_CHECK(powertrain_ids > 1350U && powertrain_ids < 1650U); // 1500 within 10 %

    powertrain.merge(chassis); // union: 1500 + 1000 - 220 = 2280 distinct ids
    const uint64_t all_ids = powertrain.estimate();
    CASTLE_SAMPLE_CHECK(all_ids > 2050U && all_ids < 2510U);

    return 0;
}
