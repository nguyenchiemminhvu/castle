#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/inf.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // Simulate the receiver emitting a UBX-INF-NOTICE (no NUL on the wire).
    inf_notice<> notice{};
    CASTLE_SAMPLE_CHECK(castle::succeeded(notice.text.assign("GNSS ready")));

    uint8_t frame[64U];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(notice, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == (UBX_FRAME_OVERHEAD + 10U));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));

    // Per-severity models only match their own ID.
    CASTLE_SAMPLE_CHECK(inf_notice<>::matches(raw));
    CASTLE_SAMPLE_CHECK(!inf_error<>::matches(raw));
    CASTLE_SAMPLE_CHECK(!inf_warning<>::matches(raw));
    CASTLE_SAMPLE_CHECK(!inf_test<>::matches(raw));
    CASTLE_SAMPLE_CHECK(!inf_debug<>::matches(raw));
    CASTLE_SAMPLE_CHECK(inf_notice<>::subtype == inf_subtype::notice);

    inf_notice<> decoded_notice{};
    CASTLE_SAMPLE_CHECK(inf_notice<>::decode(raw, decoded_notice));
    CASTLE_SAMPLE_CHECK(decoded_notice.text == castle::container::string_view("GNSS ready"));

    // The any-severity model records which ID arrived.
    inf<> any{};
    CASTLE_SAMPLE_CHECK(inf<>::matches(raw));
    CASTLE_SAMPLE_CHECK(inf<>::decode(raw, any));
    CASTLE_SAMPLE_CHECK(any.message_id == UBX_ID_INF_NOTICE);
    CASTLE_SAMPLE_CHECK(any.subtype == inf_subtype::notice);
    CASTLE_SAMPLE_CHECK(any.text.size() == 10U);

    // A small capacity keeps only the first characters.
    inf<4U> clipped{};
    CASTLE_SAMPLE_CHECK(inf<4U>::decode(raw, clipped));
    CASTLE_SAMPLE_CHECK(clipped.text == castle::container::string_view("GNSS"));

    // An empty INF string is valid.
    inf_error<> empty{};
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(empty, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(inf_error<>::decode(raw, empty));
    CASTLE_SAMPLE_CHECK(empty.text.empty());

    // Other classes are rejected.
    message_view other = raw;
    other.header.msg_class = UBX_CLASS_CFG;
    CASTLE_SAMPLE_CHECK(!inf<>::matches(other));
    CASTLE_SAMPLE_CHECK(!inf<>::decode(other, any));

    return 0;
}
