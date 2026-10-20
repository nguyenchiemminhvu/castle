#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/detail/nav_gnss_time.hpp"
#include "castle_ext/protocols/ubx/messages/nav_timebds.hpp"
#include "castle_ext/protocols/ubx/messages/nav_timegal.hpp"
#include "castle_ext/protocols/ubx/messages/nav_timeqzss.hpp"

namespace
{
bool near(double value, double expected, double tolerance)
{
    const double difference = value - expected;
    return (difference <= tolerance) && (difference >= -tolerance);
}

/** Fills the generic fields of any nav_gnss_time-derived message the same way. */
template <typename Message>
Message make_time()
{
    Message value{};
    value.i_tow = 345600000U;
    value.tow = 345618U;
    value.f_tow = -250000000;
    value.week = 1333;
    value.leap_s = 18;
    value.valid = static_cast<uint8_t>(Message::VALID_TOW | Message::VALID_WEEK | Message::VALID_LEAP_S);
    value.t_acc = 20U;
    return value;
}
} // namespace

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // The three messages share one 20-byte layout but have distinct IDs.
    CASTLE_SAMPLE_CHECK(nav_timebds::payload_length == 20U);
    CASTLE_SAMPLE_CHECK(nav_timegal::payload_length == nav_timebds::payload_length);
    CASTLE_SAMPLE_CHECK(nav_timeqzss::payload_length == nav_timebds::payload_length);
    CASTLE_SAMPLE_CHECK(nav_timebds::msg_class == UBX_CLASS_NAV);
    CASTLE_SAMPLE_CHECK(nav_timegal::msg_class == UBX_CLASS_NAV);
    CASTLE_SAMPLE_CHECK(nav_timeqzss::msg_class == UBX_CLASS_NAV);
    CASTLE_SAMPLE_CHECK(nav_timebds::msg_id == UBX_ID_NAV_TIMEBDS);
    CASTLE_SAMPLE_CHECK(nav_timegal::msg_id == UBX_ID_NAV_TIMEGAL);
    CASTLE_SAMPLE_CHECK(nav_timeqzss::msg_id == UBX_ID_NAV_TIMEQZSS);
    CASTLE_SAMPLE_CHECK(nav_timebds::msg_id != nav_timegal::msg_id);
    CASTLE_SAMPLE_CHECK(nav_timegal::msg_id != nav_timeqzss::msg_id);
    CASTLE_SAMPLE_CHECK(nav_timeqzss::msg_id != nav_timebds::msg_id);

    // Same generic field values written through each message.
    const nav_timebds bds = make_time<nav_timebds>();
    const nav_timegal gal = make_time<nav_timegal>();
    const nav_timeqzss qzss = make_time<nav_timeqzss>();

    uint8_t bds_frame[UBX_FRAME_OVERHEAD + nav_timebds::payload_length];
    uint8_t gal_frame[UBX_FRAME_OVERHEAD + nav_timegal::payload_length];
    uint8_t qzss_frame[UBX_FRAME_OVERHEAD + nav_timeqzss::payload_length];
    castle::size_type bds_written = 0U;
    castle::size_type gal_written = 0U;
    castle::size_type qzss_written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(bds, bds_frame, sizeof(bds_frame), bds_written)));
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(gal, gal_frame, sizeof(gal_frame), gal_written)));
    CASTLE_SAMPLE_CHECK(castle::succeeded(encode_message(qzss, qzss_frame, sizeof(qzss_frame), qzss_written)));
    CASTLE_SAMPLE_CHECK(bds_written == sizeof(bds_frame));
    CASTLE_SAMPLE_CHECK(gal_written == bds_written);
    CASTLE_SAMPLE_CHECK(qzss_written == bds_written);

    // Shared layout: the payload bytes are identical; only the header ID differs.
    for (castle::size_type i = 0U; i < nav_timebds::payload_length; ++i)
    {
        CASTLE_SAMPLE_CHECK(bds_frame[UBX_FRAME_INDEX_PAYLOAD + i] == gal_frame[UBX_FRAME_INDEX_PAYLOAD + i]);
        CASTLE_SAMPLE_CHECK(bds_frame[UBX_FRAME_INDEX_PAYLOAD + i] == qzss_frame[UBX_FRAME_INDEX_PAYLOAD + i]);
    }
    CASTLE_SAMPLE_CHECK(bds_frame[UBX_FRAME_INDEX_ID] == UBX_ID_NAV_TIMEBDS);
    CASTLE_SAMPLE_CHECK(gal_frame[UBX_FRAME_INDEX_ID] == UBX_ID_NAV_TIMEGAL);
    CASTLE_SAMPLE_CHECK(qzss_frame[UBX_FRAME_INDEX_ID] == UBX_ID_NAV_TIMEQZSS);

    // Distinct IDs: each frame matches only its own message.
    message_view bds_raw{};
    message_view gal_raw{};
    message_view qzss_raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(bds_frame, bds_written), bds_raw));
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(gal_frame, gal_written), gal_raw));
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(qzss_frame, qzss_written), qzss_raw));
    CASTLE_SAMPLE_CHECK(nav_timebds::matches(bds_raw) && !nav_timegal::matches(bds_raw) && !nav_timeqzss::matches(bds_raw));
    CASTLE_SAMPLE_CHECK(!nav_timebds::matches(gal_raw) && nav_timegal::matches(gal_raw) && !nav_timeqzss::matches(gal_raw));
    CASTLE_SAMPLE_CHECK(!nav_timebds::matches(qzss_raw) && !nav_timegal::matches(qzss_raw) && nav_timeqzss::matches(qzss_raw));

    nav_timegal wrong{};
    CASTLE_SAMPLE_CHECK(!nav_timegal::decode(bds_raw, wrong));

    // Decode each frame into its own model.
    nav_timebds bds_decoded{};
    nav_timegal gal_decoded{};
    nav_timeqzss qzss_decoded{};
    CASTLE_SAMPLE_CHECK(nav_timebds::decode(bds_raw, bds_decoded));
    CASTLE_SAMPLE_CHECK(nav_timegal::decode(gal_raw, gal_decoded));
    CASTLE_SAMPLE_CHECK(nav_timeqzss::decode(qzss_raw, qzss_decoded));

    // Generic fields are the storage; spec-named accessors read the same members.
    CASTLE_SAMPLE_CHECK(bds_decoded.i_tow == 345600000U);
    CASTLE_SAMPLE_CHECK(bds_decoded.tow == 345618U);
    CASTLE_SAMPLE_CHECK(bds_decoded.f_tow == -250000000);
    CASTLE_SAMPLE_CHECK(bds_decoded.week == 1333);
    CASTLE_SAMPLE_CHECK(bds_decoded.leap_s == 18);
    CASTLE_SAMPLE_CHECK(bds_decoded.t_acc == 20U);
    CASTLE_SAMPLE_CHECK(bds_decoded.sow() == bds_decoded.tow);           // SOW
    CASTLE_SAMPLE_CHECK(bds_decoded.f_sow() == bds_decoded.f_tow);       // fSOW
    CASTLE_SAMPLE_CHECK(gal_decoded.gal_tow() == gal_decoded.tow);       // galTow
    CASTLE_SAMPLE_CHECK(gal_decoded.f_gal_tow() == gal_decoded.f_tow);   // fGalTow
    CASTLE_SAMPLE_CHECK(gal_decoded.gal_wno() == gal_decoded.week);      // galWno
    CASTLE_SAMPLE_CHECK(qzss_decoded.qzss_tow() == qzss_decoded.tow);    // qzssTow
    CASTLE_SAMPLE_CHECK(qzss_decoded.f_qzss_tow() == qzss_decoded.f_tow); // fQzssTow
    CASTLE_SAMPLE_CHECK(qzss_decoded.qzss_wno() == qzss_decoded.week);   // qzssWno

    // Valid bits: spec-named masks alias the generic ones.
    CASTLE_SAMPLE_CHECK(nav_timebds::VALID_SOW == nav_timebds::VALID_TOW);
    CASTLE_SAMPLE_CHECK(nav_timegal::VALID_GAL_TOW == nav_timegal::VALID_TOW);
    CASTLE_SAMPLE_CHECK(nav_timegal::VALID_GAL_WNO == nav_timegal::VALID_WEEK);
    CASTLE_SAMPLE_CHECK(nav_timeqzss::VALID_QZSS_TOW == nav_timeqzss::VALID_TOW);
    CASTLE_SAMPLE_CHECK(nav_timeqzss::VALID_QZSS_WNO == nav_timeqzss::VALID_WEEK);
    CASTLE_SAMPLE_CHECK(bds_decoded.sow_valid() && bds_decoded.tow_valid());
    CASTLE_SAMPLE_CHECK(bds_decoded.week_valid() && bds_decoded.leap_s_valid());
    CASTLE_SAMPLE_CHECK(gal_decoded.gal_tow_valid() && gal_decoded.gal_wno_valid() && gal_decoded.leap_s_valid());
    CASTLE_SAMPLE_CHECK(qzss_decoded.qzss_tow_valid() && qzss_decoded.qzss_wno_valid() && qzss_decoded.leap_s_valid());

    gal_decoded.valid = nav_timegal::VALID_LEAP_S;
    CASTLE_SAMPLE_CHECK(!gal_decoded.gal_tow_valid() && !gal_decoded.tow_valid());
    CASTLE_SAMPLE_CHECK(!gal_decoded.gal_wno_valid() && !gal_decoded.week_valid());
    CASTLE_SAMPLE_CHECK(gal_decoded.leap_s_valid());

    // time_of_week_s() = tow + f_tow * 1e-9, identical for all three.
    CASTLE_SAMPLE_CHECK(near(bds_decoded.time_of_week_s(), 345617.75, 1e-9));
    CASTLE_SAMPLE_CHECK(near(gal_decoded.time_of_week_s(), 345617.75, 1e-9));
    CASTLE_SAMPLE_CHECK(near(qzss_decoded.time_of_week_s(), 345617.75, 1e-9));

    return 0;
}
