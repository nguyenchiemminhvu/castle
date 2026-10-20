#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/mon_rf.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // One RF block per band: L1 healthy, L2 with interference visible.
    mon_rf<> report{};
    mon_rf_block l1{};
    l1.block_id = mon_rf_block::BLOCK_ID_L1;
    l1.flags = static_cast<uint8_t>(mon_jamming_state::ok);
    l1.ant_status = mon_antenna_status::ok;
    l1.ant_power = mon_antenna_power::on;
    l1.noise_per_ms = 85U;
    l1.agc_cnt = 6000U;
    l1.ofs_i = -4;
    l1.mag_i = 150U;
    l1.ofs_q = 3;
    l1.mag_q = 149U;
    mon_rf_block l2 = l1;
    l2.block_id = mon_rf_block::BLOCK_ID_L2_L5;
    l2.flags = static_cast<uint8_t>(mon_jamming_state::warning);
    l2.jam_ind = 90U;
    CASTLE_SAMPLE_CHECK(report.blocks.push_back(l1) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(report.blocks.push_back(l2) == castle::status::ok);

    uint8_t frame[64U] = {};
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(report, frame, sizeof(frame), written) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(written == frame_size(mon_rf<>::header_length + (2U * mon_rf<>::block_length)));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(mon_rf<>::matches(raw));

    mon_rf<> decoded{};
    CASTLE_SAMPLE_CHECK(mon_rf<>::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.n_blocks == 2U);
    CASTLE_SAMPLE_CHECK(decoded.blocks.size() == 2U);
    for (const mon_rf_block& block : decoded.blocks)
    {
        CASTLE_SAMPLE_CHECK(block.ant_status == mon_antenna_status::ok);
        CASTLE_SAMPLE_CHECK(block.ant_power == mon_antenna_power::on);
        CASTLE_SAMPLE_CHECK(block.jamming_state() != mon_jamming_state::critical);
    }
    CASTLE_SAMPLE_CHECK(decoded.blocks[1U].jamming_state() == mon_jamming_state::warning);
    CASTLE_SAMPLE_CHECK(decoded.blocks[1U].jam_ind == 90U);
    CASTLE_SAMPLE_CHECK(decoded.blocks[0U].ofs_i == -4);

    // A smaller model keeps the on-wire count but stores only what fits.
    mon_rf<1U> l1_only{};
    CASTLE_SAMPLE_CHECK(mon_rf<1U>::decode(raw, l1_only));
    CASTLE_SAMPLE_CHECK(l1_only.n_blocks == 2U);
    CASTLE_SAMPLE_CHECK(l1_only.blocks.size() == 1U);

    CASTLE_SAMPLE_CHECK(encode_message(report, frame, frame_size(4U), written) == castle::status::full);
    return 0;
}
