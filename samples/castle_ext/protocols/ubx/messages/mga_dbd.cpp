#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/mga_dbd.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // 1. Poll the navigation database: empty payload, no struct needed.
    uint8_t poll[UBX_FRAME_OVERHEAD];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_poll<mga_dbd<>>(poll, sizeof(poll), written) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(written == UBX_FRAME_OVERHEAD);
    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(poll, written), raw));
    CASTLE_SAMPLE_CHECK(raw.is(UBX_CLASS_MGA, UBX_ID_MGA_DBD));
    CASTLE_SAMPLE_CHECK(!mga_dbd<>::matches(raw)); // the poll is not a database entry

    // 2. A database entry as the receiver would send it (12 reserved bytes + data).
    uint8_t payload[12U + 6U] = {};
    for (castle::size_type i = 0U; i < 6U; ++i)
    {
        payload[12U + i] = static_cast<uint8_t>(0xB0U + i);
    }
    uint8_t frame[12U + 164U + UBX_FRAME_OVERHEAD];
    CASTLE_SAMPLE_CHECK(encode_frame(UBX_CLASS_MGA, UBX_ID_MGA_DBD,
                                     castle::container::array_view<const uint8_t>(payload, sizeof(payload)),
                                     frame, sizeof(frame), written) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(mga_dbd<>::matches(raw));

    mga_dbd<> entry{};
    CASTLE_SAMPLE_CHECK(mga_dbd<>::decode(raw, entry));
    CASTLE_SAMPLE_CHECK(entry.data.size() == 6U);
    CASTLE_SAMPLE_CHECK(entry.data[0U] == 0xB0U && entry.data[5U] == 0xB5U);

    // 3. Send the stored entry back to the same receiver (loop-back).
    CASTLE_SAMPLE_CHECK(encode_message(entry, frame, sizeof(frame), written) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(written == sizeof(payload) + UBX_FRAME_OVERHEAD);
    CASTLE_SAMPLE_CHECK(frame[UBX_FRAME_INDEX_PAYLOAD + 12U] == 0xB0U);

    // A smaller capacity keeps only the first bytes.
    mga_dbd<2U> small{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(mga_dbd<2U>::decode(raw, small));
    CASTLE_SAMPLE_CHECK(small.data.size() == 2U && small.data[1U] == 0xB1U);

    CASTLE_SAMPLE_CHECK(encode_message(entry, frame, 12U, written) == castle::status::full);

    return 0;
}
