#include <assert.h>
#include <stdint.h>
#include "castle_ext/checksums/fletcher16.hpp"

int main()
{
    const uint8_t data[] = {'1','2','3','4','5','6','7','8','9'};
    castle::checksums::fletcher16 checksum;
    checksum.update(data, 2U);
    checksum.update(data + 2U, sizeof(data) - 2U);
    assert(checksum.value() == 0x1EDEU);
    assert(checksum.checksum() == 0x1EDEU);
    assert(castle::checksums::fletcher16::calculate(data, sizeof(data)) == 0x1EDEU);
    assert(castle::checksums::fletcher16::calculate(static_cast<const void*>(data), sizeof(data)) == 0x1EDEU);
    checksum.reset();
    assert(checksum.value() == 0U);
    return 0;
}
