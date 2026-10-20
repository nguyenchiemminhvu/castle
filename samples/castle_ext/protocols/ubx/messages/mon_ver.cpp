#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/mon_ver.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using castle::protocols::ubx::messages::mon_ver;
    using castle::container::string_view;

    // Poll the version (empty-payload request).
    uint8_t frame[256U];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_poll<mon_ver<4U>>(frame, sizeof(frame), written) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(written == UBX_FRAME_OVERHEAD);

    // A receiver answer with two extension strings.
    mon_ver<4U> version{};
    version.sw_version = "EXT CORE 1.00 (a59682)";
    version.hw_version = "00190000";
    CASTLE_SAMPLE_CHECK(version.extensions.push_back("ROM BASE 0x118B2060") == castle::status::ok);
    CASTLE_SAMPLE_CHECK(version.extensions.push_back("FWVER=HPS 1.30") == castle::status::ok);

    CASTLE_SAMPLE_CHECK(encode_message(version, frame, sizeof(frame), written) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(written == frame_size(mon_ver<4U>::header_length + (2U * mon_ver<4U>::block_length)));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(mon_ver<4U>::matches(raw));

    mon_ver<4U> decoded{};
    CASTLE_SAMPLE_CHECK(mon_ver<4U>::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.sw_version == string_view("EXT CORE 1.00 (a59682)"));
    CASTLE_SAMPLE_CHECK(decoded.hw_version == string_view("00190000"));
    CASTLE_SAMPLE_CHECK(decoded.extensions.size() == 2U);
    CASTLE_SAMPLE_CHECK(decoded.extensions[0U] == string_view("ROM BASE 0x118B2060"));
    CASTLE_SAMPLE_CHECK(decoded.extensions[1U] == string_view("FWVER=HPS 1.30"));

    // Storage for fewer extensions than the receiver reports: the rest are skipped.
    mon_ver<1U> first_extension{};
    CASTLE_SAMPLE_CHECK(mon_ver<1U>::decode(raw, first_extension));
    CASTLE_SAMPLE_CHECK(first_extension.extensions.size() == 1U);
    CASTLE_SAMPLE_CHECK(first_extension.extensions[0U] == string_view("ROM BASE 0x118B2060"));

    // Too small an output buffer is reported, not overrun.
    CASTLE_SAMPLE_CHECK(encode_message(version, frame, written - 1U, written) == castle::status::full);
    return 0;
}
