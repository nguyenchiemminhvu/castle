#include "sample_support.hpp"

#include "castle/dsa/cuckoo_filter.hpp"

int main()
{
    using castle::status;

    // 1) Diagnostic gateway: track open tester sessions by source address. Sessions open and
    //    close at runtime, so the set needs erase(), which a Bloom filter cannot do.
    castle::dsa::cuckoo_filter_for<uint32_t, 32U> sessions; // 16 buckets x 4 slots, 128 bytes
    const uint32_t tester = 0x0E80U;
    const uint32_t flasher = 0x0E81U;
    CASTLE_SAMPLE_CHECK(sessions.insert_unique(tester) == status::ok);
    CASTLE_SAMPLE_CHECK(sessions.insert_unique(flasher) == status::ok);
    CASTLE_SAMPLE_CHECK(sessions.insert_unique(tester) == status::already_exists); // repeated open request

    CASTLE_SAMPLE_CHECK(sessions.contains(tester));  // request from an open session: forward it
    CASTLE_SAMPLE_CHECK(!sessions.contains(0x0E99U)); // unknown source: reject without a table search

    CASTLE_SAMPLE_CHECK(sessions.erase(tester) == status::ok); // session closed
    CASTLE_SAMPLE_CHECK(!sessions.contains(tester));
    CASTLE_SAMPLE_CHECK(sessions.contains(flasher));

    // 2) Drop duplicate frames inside a sliding window of the last 16 sequence numbers:
    //    insert a number when it enters the window, erase it when it leaves.
    castle::dsa::cuckoo_filter<uint32_t, 8U, 4U, uint16_t> window; // 32 slots for 16 entries
    uint32_t history[16] = {};
    uint32_t accepted = 0U;
    uint32_t dropped = 0U;
    for (uint32_t tick = 0U; tick < 200U; ++tick)
    {
        // Every fifth frame is a retransmission of the previous one.
        const uint32_t sequence = ((tick % 5U) == 4U) ? (tick - 1U) : tick;
        if (window.contains(sequence))
        {
            ++dropped;
            continue;
        }
        if (accepted >= 16U)
        {
            CASTLE_SAMPLE_CHECK(window.erase(history[accepted % 16U]) == status::ok); // oldest leaves
        }
        CASTLE_SAMPLE_CHECK(window.insert(sequence) == status::ok);
        history[accepted % 16U] = sequence;
        ++accepted;
    }
    CASTLE_SAMPLE_CHECK(dropped == 40U);   // every retransmission caught
    CASTLE_SAMPLE_CHECK(accepted == 160U); // 16-bit fingerprints: no new frame dropped by mistake
    CASTLE_SAMPLE_CHECK(window.size() == 16U);

    return 0;
}
