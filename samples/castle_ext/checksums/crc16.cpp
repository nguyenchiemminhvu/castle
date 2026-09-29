#include <assert.h>
#include <stdint.h>
#include "castle_ext/checksums/crc16.hpp"

int main()
{
    const uint8_t data[] = {'1','2','3','4','5','6','7','8','9'};
    castle::checksums::crc16 crc;
    crc.update(data, 3U);
    crc.update(data + 3U, 6U);
    assert(crc.value() == 0x29B1U);
    assert(crc.checksum() == 0x29B1U);
    assert(castle::checksums::crc16::calculate(data, sizeof(data)) == 0x29B1U);
    assert(castle::checksums::crc16::calculate(static_cast<const void*>(data), sizeof(data)) == 0x29B1U);
    crc.reset();
    assert(crc.value() == 0xFFFFU);
    return 0;
}
