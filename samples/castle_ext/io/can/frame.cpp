#include "sample_support.hpp"

#include "castle_ext/io/can/frame.hpp"

using namespace castle::io::can;

// frame is a plain value type, so it can be built at compile time.
static constexpr frame make_heartbeat()
{
    return frame(0x701U);
}

int main()
{
    static_assert(max_data_length == 8U, "classic CAN carries at most 8 bytes");
    static_assert(max_id(id_format::standard) == max_standard_id, "11-bit identifiers");
    static_assert(max_id(id_format::extended) == max_extended_id, "29-bit identifiers");
    static_assert(make_heartbeat().id == 0x701U, "frames are usable in constant expressions");

    // A default frame is an empty standard-format data frame with identifier 0.
    frame blank;
    CASTLE_SAMPLE_CHECK(blank.id == 0U);
    CASTLE_SAMPLE_CHECK(blank.format == id_format::standard);
    CASTLE_SAMPLE_CHECK(blank.type == frame_type::data);
    CASTLE_SAMPLE_CHECK(blank.dlc == 0U);
    CASTLE_SAMPLE_CHECK(blank.data_length() == 0U);
    CASTLE_SAMPLE_CHECK(blank.payload().empty());
    CASTLE_SAMPLE_CHECK(!blank.is_extended());
    CASTLE_SAMPLE_CHECK(!blank.is_remote());

    // Data frame with a brace-enclosed payload (standard 11-bit identifier).
    frame request(0x123U);
    CASTLE_SAMPLE_CHECK(request.set_data({0x01U, 0x02U, 0x03U}) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(request.dlc == 3U);
    CASTLE_SAMPLE_CHECK(request.data_length() == 3U);
    CASTLE_SAMPLE_CHECK(request.payload().size() == 3U);
    CASTLE_SAMPLE_CHECK(request.payload()[0] == 0x01U);
    CASTLE_SAMPLE_CHECK(request.payload()[2] == 0x03U);
    CASTLE_SAMPLE_CHECK(request.data[3] == 0U); // bytes past the DLC stay zero

    // Raw pointer + length overload; a shorter payload clears the previous tail bytes.
    const uint8_t full_payload[max_data_length] = {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U};
    CASTLE_SAMPLE_CHECK(request.set_data(full_payload, sizeof(full_payload)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(request.data_length() == max_data_length);
    CASTLE_SAMPLE_CHECK(request.set_data(full_payload, 2U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(request.data[2] == 0U && request.data[7] == 0U);

    // Zero-length payloads are valid, and the source pointer may then be null.
    CASTLE_SAMPLE_CHECK(request.set_data(nullptr, 0U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(request.dlc == 0U);

    // Rejected input leaves the frame unchanged.
    CASTLE_SAMPLE_CHECK(request.set_data(full_payload, 2U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(request.set_data(full_payload, max_data_length + 1U) == castle::status::out_of_range);
    CASTLE_SAMPLE_CHECK(request.set_data(nullptr, 1U) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(request.dlc == 2U);
    CASTLE_SAMPLE_CHECK(request.data[1] == 2U);

    // Extended 29-bit remote request: no payload, but DLC carries the requested length.
    frame poll_request(0x18FF0001U, id_format::extended);
    CASTLE_SAMPLE_CHECK(poll_request.is_extended());
    CASTLE_SAMPLE_CHECK(poll_request.set_remote(4U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(poll_request.is_remote());
    CASTLE_SAMPLE_CHECK(poll_request.type == frame_type::remote);
    CASTLE_SAMPLE_CHECK(poll_request.dlc == 4U);
    CASTLE_SAMPLE_CHECK(poll_request.data_length() == 0U);
    CASTLE_SAMPLE_CHECK(poll_request.payload().empty());
    CASTLE_SAMPLE_CHECK(poll_request.set_remote(max_data_length + 1U) == castle::status::out_of_range);
    CASTLE_SAMPLE_CHECK(poll_request.dlc == 4U);

    // set_data() turns a remote frame back into a data frame.
    CASTLE_SAMPLE_CHECK(poll_request.set_data({0xAAU}) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(!poll_request.is_remote());
    CASTLE_SAMPLE_CHECK(poll_request.data_length() == 1U);

    // Equality compares id, format, type, DLC and only the valid payload bytes.
    frame a(0x100U);
    frame b(0x100U);
    CASTLE_SAMPLE_CHECK(a.set_data({1U, 2U}) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(b.set_data({1U, 2U}) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(a == b);
    b.data[5] = 0x55U; // stale byte beyond the DLC is ignored
    CASTLE_SAMPLE_CHECK(a == b);
    b.data[1] = 3U;
    CASTLE_SAMPLE_CHECK(a != b);
    frame c(0x100U, id_format::extended);
    CASTLE_SAMPLE_CHECK(c.set_data({1U, 2U}) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(a != c); // same number, different identifier format

    // validate_frame() accepts well-formed frames at the identifier limits...
    CASTLE_SAMPLE_CHECK(validate_frame(frame(max_standard_id)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(validate_frame(frame(max_extended_id, id_format::extended)) == castle::status::ok);

    // ...and rejects fields that were assigned directly to inconsistent values.
    CASTLE_SAMPLE_CHECK(validate_frame(frame(max_standard_id + 1U)) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(validate_frame(frame(max_extended_id + 1U, id_format::extended))
                        == castle::status::invalid_argument);
    frame oversized(0x10U);
    oversized.dlc = static_cast<uint8_t>(max_data_length + 1U);
    CASTLE_SAMPLE_CHECK(validate_frame(oversized) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(oversized.data_length() == max_data_length); // data_length() clamps for safe reads
    frame bad_format(0x10U);
    bad_format.format = static_cast<id_format>(7U);
    CASTLE_SAMPLE_CHECK(validate_frame(bad_format) == castle::status::invalid_argument);
    frame bad_type(0x10U);
    bad_type.type = static_cast<frame_type>(7U);
    CASTLE_SAMPLE_CHECK(validate_frame(bad_type) == castle::status::invalid_argument);

    return 0;
}
