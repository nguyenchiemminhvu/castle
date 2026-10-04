#include <assert.h>
#include <stdint.h>
#include "castle_ext/checksums/crc64.hpp"

int main()
{
    const uint8_t data[] = {'1','2','3','4','5','6','7','8','9'};
    castle::checksums::crc64 crc;
    crc.update(data, sizeof(data));
    assert(crc.value() == 0x6C40DF5F0B497347ULL);
    assert(crc.checksum() == 0x6C40DF5F0B497347ULL);
    assert(castle::checksums::crc64::calculate(data, sizeof(data)) == 0x6C40DF5F0B497347ULL);
    crc.reset();
    assert(crc.value() == 0ULL);
    return 0;
}
