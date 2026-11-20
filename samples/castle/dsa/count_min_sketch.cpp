#include "sample_support.hpp"

#include "castle/dsa/count_min_sketch.hpp"
#include "castle/dsa/string_view.hpp"

int main()
{
    using castle::dsa::string_view;

    // 1) Find a babbling CAN node: count frames per identifier without one counter per id.
    //    Error <= 1 / 50 of all frames with 99 % confidence: 136 x 5 counters of 16 bits (1360 bytes).
    castle::dsa::count_min_sketch_for<uint32_t, 50U, 2U, uint16_t> frames;
    const uint16_t babble_limit = 100U;
    bool babbling = false;
    for (uint32_t tick = 0U; tick < 200U; ++tick)
    {
        frames.add(0x100U + (tick % 8U)); // eight normal nodes, 25 frames each
        if (frames.add(0x7FFU) > babble_limit) // add() returns the new estimate
        {
            babbling = true; // one node sends every tick
        }
    }
    CASTLE_SAMPLE_CHECK(babbling);
    CASTLE_SAMPLE_CHECK(frames.estimate(0x7FFU) >= 200U);   // never undercounts
    CASTLE_SAMPLE_CHECK(frames.estimate(0x103U) < babble_limit); // normal node stays below the limit

    // 2) Rate limit per source over time windows: halve() ages old traffic.
    //    Conservative update keeps the estimates tighter for the same memory.
    castle::dsa::count_min_sketch<uint32_t, 64U, 4U, uint8_t, castle::dsa::count_min_update::conservative> requests;
    const uint32_t client = 0xC0A80017U; // 192.168.0.23
    requests.add(client, 40U);
    CASTLE_SAMPLE_CHECK(requests.estimate(client) == 40U);
    requests.halve(); // end of window: old requests count half
    requests.halve();
    CASTLE_SAMPLE_CHECK(requests.estimate(client) == 10U);

    // 3) Combine event statistics from two ECUs at the gateway.
    castle::dsa::count_min_sketch<string_view, 32U, 3U, uint16_t> body_ecu;
    castle::dsa::count_min_sketch<string_view, 32U, 3U, uint16_t> door_ecu;
    body_ecu.add(string_view("low_voltage"), 3U);
    door_ecu.add(string_view("low_voltage"), 2U);
    door_ecu.add(string_view("door_ajar"));

    body_ecu.merge(door_ecu);
    CASTLE_SAMPLE_CHECK(body_ecu.estimate(string_view("low_voltage")) >= 5U);
    CASTLE_SAMPLE_CHECK(body_ecu.estimate(string_view("door_ajar")) >= 1U);
    CASTLE_SAMPLE_CHECK(body_ecu.total() == 6U);

    return 0;
}
