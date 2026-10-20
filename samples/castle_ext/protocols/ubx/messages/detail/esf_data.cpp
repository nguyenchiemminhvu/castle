#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/detail/esf_data.hpp"
#include "castle/utility/bytes.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using namespace castle::protocols::ubx::messages;

    // dataField is a two's-complement 24-bit value in bits 23..0.
    CASTLE_SAMPLE_CHECK(detail::esf_data_field(0x007FFFFFU) == detail::ESF_DATA_FIELD_MAX);
    CASTLE_SAMPLE_CHECK(detail::esf_data_field(0x00800000U) == detail::ESF_DATA_FIELD_MIN);
    CASTLE_SAMPLE_CHECK(detail::esf_data_field(0x0BFFFFFFU) == -1); // type bits are ignored

    // Encode one ESF-MEAS word (6-bit type) and one ESF-RAW word (8-bit type).
    uint8_t payload[8U] = {};
    payload_writer writer(castle::container::array_view<uint8_t>(payload, sizeof(payload)));
    detail::esf_data_word<0x3FU>(writer, static_cast<uint8_t>(11U), static_cast<int32_t>(-1234));
    detail::esf_data_word<0xFFU>(writer, static_cast<uint8_t>(0xF0U), static_cast<int32_t>(4096));
    CASTLE_SAMPLE_CHECK(writer.ok());
    CASTLE_SAMPLE_CHECK(castle::read_le32(&payload[0U]) == ((11U << 24U) | (static_cast<uint32_t>(-1234) & 0x00FFFFFFU)));
    CASTLE_SAMPLE_CHECK(castle::read_le32(&payload[4U]) == ((0xF0U << 24U) | 4096U));

    // Decode them back through the reader overload.
    payload_reader reader(castle::container::array_view<CASTLE_CONST uint8_t>(payload, sizeof(payload)));
    uint8_t type = 0U;
    int32_t value = 0;
    detail::esf_data_word<0x3FU>(reader, type, value);
    CASTLE_SAMPLE_CHECK((type == 11U) && (value == -1234));
    detail::esf_data_word<0xFFU>(reader, type, value);
    CASTLE_SAMPLE_CHECK((type == 0xF0U) && (value == 4096));
    CASTLE_SAMPLE_CHECK(reader.ok());

    // Values that do not fit are refused instead of being truncated.
    payload_writer refused(castle::container::array_view<uint8_t>(payload, sizeof(payload)));
    detail::esf_data_word<0x3FU>(refused, static_cast<uint8_t>(64U), static_cast<int32_t>(0));
    CASTLE_SAMPLE_CHECK(refused.status() == castle::status::out_of_range);
    payload_writer too_big(castle::container::array_view<uint8_t>(payload, sizeof(payload)));
    detail::esf_data_word<0xFFU>(too_big, static_cast<uint8_t>(1U), detail::ESF_DATA_FIELD_MAX + 1);
    CASTLE_SAMPLE_CHECK(too_big.status() == castle::status::out_of_range);

    return 0;
}
