#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/mon_hw3.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    mon_hw3<> status{};
    status.flags = mon_hw3<>::FLAGS_RTC_CALIB;
    CASTLE_SAMPLE_CHECK(status.hw_version.assign("00190000") == castle::status::ok);

    // Pin 0x0010: PIO, bank C, output driven high, pull-up enabled.
    mon_hw3_pin pin{};
    pin.pin_id = 0x0010U;
    pin.pin_mask = static_cast<uint16_t>(mon_hw3_pin::PIN_MASK_PERIPH_PIO | (2U << 1U) |
                                         mon_hw3_pin::PIN_MASK_DIRECTION | mon_hw3_pin::PIN_MASK_VALUE |
                                         mon_hw3_pin::PIN_MASK_PIO_PULL_HIGH);
    pin.vp = 3U;
    CASTLE_SAMPLE_CHECK(status.pins.push_back(pin) == castle::status::ok);

    uint8_t frame[64U] = {};
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(status, frame, sizeof(frame), written) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(written == frame_size(mon_hw3<>::header_length + mon_hw3<>::block_length));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(mon_hw3<>::matches(raw));

    mon_hw3<> decoded{};
    CASTLE_SAMPLE_CHECK(mon_hw3<>::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.rtc_calib());
    CASTLE_SAMPLE_CHECK(!decoded.safe_boot());
    CASTLE_SAMPLE_CHECK(!decoded.xtal_absent());
    CASTLE_SAMPLE_CHECK(decoded.hw_version == status.hw_version);
    CASTLE_SAMPLE_CHECK(decoded.n_pins == 1U);
    CASTLE_SAMPLE_CHECK(decoded.pins.size() == 1U);

    const mon_hw3_pin& out = decoded.pins[0U];
    CASTLE_SAMPLE_CHECK(out.pin_id == 0x0010U);
    CASTLE_SAMPLE_CHECK(out.periph_pio());
    CASTLE_SAMPLE_CHECK(out.pin_bank() == 2U);
    CASTLE_SAMPLE_CHECK(out.direction());
    CASTLE_SAMPLE_CHECK(out.value());
    CASTLE_SAMPLE_CHECK(!out.vp_manager());
    CASTLE_SAMPLE_CHECK(!out.pio_irq());
    CASTLE_SAMPLE_CHECK(out.pio_pull_high());
    CASTLE_SAMPLE_CHECK(!out.pio_pull_low());
    CASTLE_SAMPLE_CHECK(out.vp == 3U);

    CASTLE_SAMPLE_CHECK(encode_message(status, frame, frame_size(22U), written) == castle::status::full);
    return 0;
}
