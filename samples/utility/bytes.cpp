#include "sample_support.hpp"

#include "castle/utility/bytes.hpp"

#include <stdint.h>

int main()
{
    const uint8_t input[8] = {0x01U, 0x23U, 0x45U, 0x67U, 0x89U, 0xABU, 0xCDU, 0xEFU};
    CASTLE_SAMPLE_CHECK(castle::read_le8(input) == 0x01U);
    CASTLE_SAMPLE_CHECK(castle::read_be8(input) == 0x01U);
    CASTLE_SAMPLE_CHECK(castle::read_le16(input) == 0x2301U);
    CASTLE_SAMPLE_CHECK(castle::read_be16(input) == 0x0123U);
    CASTLE_SAMPLE_CHECK(castle::read_le32(input) == 0x67452301U);
    CASTLE_SAMPLE_CHECK(castle::read_be32(input) == 0x01234567U);
    CASTLE_SAMPLE_CHECK(castle::read_le64(input) == 0xEFCDAB8967452301ULL);
    CASTLE_SAMPLE_CHECK(castle::read_be64(input) == 0x0123456789ABCDEFULL);

    uint8_t output[8] = {};
    castle::write_be64(output, castle::read_be64(input));
    CASTLE_SAMPLE_CHECK(castle::read_be64(output) == castle::read_be64(input));
    return 0;
}