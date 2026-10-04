#include <assert.h>
#include <stdint.h>
#include "castle_ext/checksums/crc8.hpp"

int main()
{
    const uint8_t data[] = {'1','2','3','4','5','6','7','8','9'};
    castle::checksums::crc8 crc;
    crc.update(data, 4U);
    crc.update(data + 4U, 5U);
    assert(crc.value() == 0xF4U);
    assert(crc.checksum() == 0xF4U);
    assert(castle::checksums::crc8::calculate(data, sizeof(data)) == 0xF4U);
    assert(castle::checksums::crc8::calculate(static_cast<const void*>(data), sizeof(data)) == 0xF4U);
    crc.reset();
    assert(crc.value() == 0U);
    return 0;
}
