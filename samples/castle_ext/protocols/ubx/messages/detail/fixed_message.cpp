#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/detail/fixed_message.hpp"

namespace
{

namespace ubx = castle::protocols::ubx;

/** @brief UBX-NAV-EOE modelled in four lines: the base supplies matches/decode/encode. */
struct end_of_epoch : ubx::messages::detail::fixed_message<end_of_epoch, ubx::UBX_CLASS_NAV, ubx::UBX_ID_NAV_EOE, 4U>
{
    uint32_t i_tow = 0U;

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.i_tow);
    }
};

} // namespace

int main()
{
    using castle::container::array_view;

    CASTLE_SAMPLE_CHECK(end_of_epoch::msg_class == ubx::UBX_CLASS_NAV);
    CASTLE_SAMPLE_CHECK(end_of_epoch::msg_id == ubx::UBX_ID_NAV_EOE);
    CASTLE_SAMPLE_CHECK(end_of_epoch::payload_length == 4U);

    // Encode with the same layout used for decoding.
    end_of_epoch in{};
    in.i_tow = 345600000U;

    uint8_t frame[16U] = {};
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(ubx::encode_message(in, frame, sizeof(frame), written)));
    CASTLE_SAMPLE_CHECK(written == ubx::frame_size(4U));

    ubx::message_view raw;
    CASTLE_SAMPLE_CHECK(ubx::decode_frame(array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(end_of_epoch::matches(raw));

    end_of_epoch out{};
    CASTLE_SAMPLE_CHECK(end_of_epoch::decode(raw, out));
    CASTLE_SAMPLE_CHECK(out.i_tow == in.i_tow);

    // A payload of the wrong length is rejected and the output is reset.
    ubx::message_view truncated = raw;
    truncated.header.payload_length = 3U;
    out.i_tow = 1U;
    CASTLE_SAMPLE_CHECK(!end_of_epoch::decode(truncated, out));
    CASTLE_SAMPLE_CHECK(out.i_tow == 0U);

    return 0;
}
