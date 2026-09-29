#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/mon_ver.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // Construct a UBX-MON-VER payload:
    // [30 bytes SW version][10 bytes HW version][30 bytes extension]
    uint8_t payload[70U] = {};

    CASTLE_CONST char sw[] = "ROM CORE 1.00";
    for (castle::size_type i = 0U; (i < (sizeof(sw) - 1U)) && (i < 30U); ++i)
    {
        payload[i] = static_cast<uint8_t>(sw[i]);
    }

    CASTLE_CONST char hw[] = "HW-1234";
    for (castle::size_type i = 0U; (i < (sizeof(hw) - 1U)) && (i < 10U); ++i)
    {
        payload[30U + i] = static_cast<uint8_t>(hw[i]);
    }

    CASTLE_CONST char ext[] = "PROTVER=27.12";
    for (castle::size_type i = 0U; (i < (sizeof(ext) - 1U)) && (i < 30U); ++i)
    {
        payload[40U + i] = static_cast<uint8_t>(ext[i]);
    }

    // Create a validated message_view as a decoder input.
    message_view raw;
    raw.header.msg_class = mon_ver<>::msg_class;
    raw.header.msg_id = mon_ver<>::msg_id;
    raw.header.payload_length = sizeof(payload);
    raw.payload = castle::container::array_view<const uint8_t>(
        payload,
        sizeof(payload));

    CASTLE_SAMPLE_CHECK(mon_ver<>::matches(raw));

    mon_ver<> version;
    bool decoded = mon_ver<>::decode(raw, version);

    CASTLE_SAMPLE_CHECK(decoded);
    CASTLE_SAMPLE_CHECK(version.extension_count == 1U);

    CASTLE_SAMPLE_CHECK(version.sw_version[0U] == 'R');
    CASTLE_SAMPLE_CHECK(version.sw_version[1U] == 'O');
    CASTLE_SAMPLE_CHECK(version.sw_version[2U] == 'M');

    CASTLE_SAMPLE_CHECK(version.hw_version[0U] == 'H');
    CASTLE_SAMPLE_CHECK(version.hw_version[1U] == 'W');

    CASTLE_SAMPLE_CHECK(version.extensions[0U][0U] == 'P');
    CASTLE_SAMPLE_CHECK(version.extensions[0U][1U] == 'R');
    CASTLE_SAMPLE_CHECK(version.extensions[0U][2U] == 'O');

    // Wrong message type does not match.
    raw.header.msg_id = UBX_ID_MON_IO;
    CASTLE_SAMPLE_CHECK(!mon_ver<>::matches(raw));

    // Payload shorter than the mandatory 40-byte MON-VER header is rejected.
    message_view short_message;
    short_message.header.msg_class = UBX_CLASS_MON;
    short_message.header.msg_id = UBX_ID_MON_VER;
    short_message.header.payload_length = 39U;
    short_message.payload =
        castle::container::array_view<const uint8_t>(payload, 39U);

    mon_ver<> rejected;
    CASTLE_SAMPLE_CHECK(!mon_ver<>::decode(short_message, rejected));

    return 0;
}
