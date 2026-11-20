#include "sample_support.hpp"

#include "castle/dsa/string_view.hpp"
#include "castle/dsa/top_k.hpp"

int main()
{
    using castle::dsa::array_view;
    using castle::dsa::string_view;

    // 1) Top talkers on a CAN bus: 4 slots summarize a stream of 52 distinct identifiers.
    using talkers_type = castle::dsa::top_k<uint32_t, 4U, uint16_t>;
    talkers_type talkers;
    for (uint32_t tick = 0U; tick < 100U; ++tick)
    {
        talkers.add(0x7DFU);        // diagnostic tester floods the bus
        talkers.add(0x100U, 2U);    // engine status, every tick, two frames
        if ((tick % 2U) == 0U)
        {
            talkers.add(0x300U + tick); // 50 rare body ids, once each
        }
    }

    talkers_type::entry_type busiest[2];
    const castle::size_type listed = talkers.top(array_view<talkers_type::entry_type>(busiest, 2U));
    CASTLE_SAMPLE_CHECK(listed == 2U);
    CASTLE_SAMPLE_CHECK(busiest[0].key == 0x100U); // 200 frames, highest first
    CASTLE_SAMPLE_CHECK(busiest[1].key == 0x7DFU); // 100 frames
    CASTLE_SAMPLE_CHECK(busiest[1].guaranteed() <= 100U && busiest[1].count >= 100U);

    // 0x7DF certainly outranks every id that is not listed: its lower bound beats min_count().
    CASTLE_SAMPLE_CHECK(busiest[1].guaranteed() > talkers.min_count());

    // 2) Most frequent fault codes per drive cycle: halve() at the end of a cycle ages old faults.
    castle::dsa::top_k<string_view, 3U, uint8_t> faults;
    faults.add(string_view("P0300"), 12U); // misfire
    faults.add(string_view("U0100"), 3U);  // lost communication
    faults.add(string_view("P0420"));      // catalyst
    CASTLE_SAMPLE_CHECK(faults.estimate(string_view("P0300")) == 12U);

    faults.halve(); // next drive cycle: earlier faults count half
    faults.add(string_view("U0100"), 10U);
    CASTLE_SAMPLE_CHECK(faults.estimate(string_view("P0300")) == 6U);
    CASTLE_SAMPLE_CHECK(faults.estimate(string_view("U0100")) == 11U); // now the top fault

    return 0;
}
