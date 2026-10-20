#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/detail/typed_message.hpp"

namespace
{

namespace ubx = castle::protocols::ubx;

static CASTLE_CONSTEXPR uint8_t SAMPLE_CLASS = 0x7AU;
static CASTLE_CONSTEXPR uint8_t SAMPLE_ID = 0x01U;

/** @brief Layout selected by type byte 0x01. */
struct position_hint : ubx::messages::detail::typed_message<position_hint, SAMPLE_CLASS, SAMPLE_ID, 0x01U, 8U>
{
    int32_t altitude_cm = 0;

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(0x01U); // type
        stream.constant(0x00U); // version
        stream.reserved(2U);
        stream.field(self.altitude_cm);
    }
};

/** @brief Layout selected by type byte 0x02 on the same class/ID. */
struct drift_hint : ubx::messages::detail::typed_message<drift_hint, SAMPLE_CLASS, SAMPLE_ID, 0x02U, 8U>
{
    int32_t drift_ns_per_s = 0;

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.constant(0x02U);
        stream.constant(0x00U);
        stream.reserved(2U);
        stream.field(self.drift_ns_per_s);
    }
};

} // namespace

int main()
{
    using castle::container::array_view;

    CASTLE_SAMPLE_CHECK(position_hint::message_type == 0x01U);
    CASTLE_SAMPLE_CHECK(drift_hint::message_type == 0x02U);

    drift_hint in{};
    in.drift_ns_per_s = -2500;

    uint8_t frame[24U] = {};
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(ubx::encode_message(in, frame, sizeof(frame), written)));

    ubx::message_view raw;
    CASTLE_SAMPLE_CHECK(ubx::decode_frame(array_view<const uint8_t>(frame, written), raw));

    // Same class/ID, but only the drift layout claims the frame.
    CASTLE_SAMPLE_CHECK(drift_hint::matches(raw));
    CASTLE_SAMPLE_CHECK(!position_hint::matches(raw));

    drift_hint out{};
    CASTLE_SAMPLE_CHECK(drift_hint::decode(raw, out));
    CASTLE_SAMPLE_CHECK(out.drift_ns_per_s == -2500);

    // Decoding with the wrong layout fails on the type constant.
    position_hint wrong{};
    CASTLE_SAMPLE_CHECK(!position_hint::decode(raw, wrong));

    return 0;
}
