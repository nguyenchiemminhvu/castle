#include <assert.h>
#include <stdint.h>
#include "castle_ext/checksums/crc32.hpp"

int main()
{
    const uint8_t data[] = {'1','2','3','4','5','6','7','8','9'};
    castle::checksums::crc32 crc;
    crc.update(data, sizeof(data));
    assert(crc.value() == 0xCBF43926UL);
    assert(crc.checksum() == 0xCBF43926UL);
    assert(castle::checksums::crc32::calculate(data, sizeof(data)) == 0xCBF43926UL);
    crc.reset();
    assert(crc.value() == 0U);
    return 0;
}
