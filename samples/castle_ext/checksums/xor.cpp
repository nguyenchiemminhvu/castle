#include <assert.h>
#include <stdint.h>
#include "castle_ext/checksums/xor.hpp"

int main()
{
    const uint8_t data[] = {'1','2','3','4','5','6','7','8','9'};
    castle::checksums::xor_checksum checksum(0xFFU);
    checksum.update(data, sizeof(data));
    assert(checksum.value() == 0xCEU);
    assert(checksum.checksum() == 0xCEU);
    assert(castle::checksums::xor_checksum::calculate(data, sizeof(data)) == 0x31U);
    assert(castle::checksums::xor_checksum::calculate(static_cast<const void*>(data), sizeof(data), 0xFFU) == 0xCEU);
    checksum.reset();
    assert(checksum.value() == 0U);
    return 0;
}
