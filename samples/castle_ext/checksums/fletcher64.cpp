#include <assert.h>
#include <stdint.h>
#include "castle_ext/checksums/fletcher64.hpp"

int main()
{
    const uint8_t data[] = {'1','2','3','4','5','6','7','8','9'};
    castle::checksums::fletcher64 checksum;
    checksum.update(data, 5U);
    checksum.update(data + 5U, sizeof(data) - 5U);
    assert(checksum.value() == 0x0D0803376C6A689FULL);
    assert(checksum.checksum() == 0x0D0803376C6A689FULL);
    assert(castle::checksums::fletcher64::calculate(data, sizeof(data)) == 0x0D0803376C6A689FULL);
    assert(castle::checksums::fletcher64::calculate(static_cast<const void*>(data), sizeof(data)) == 0x0D0803376C6A689FULL);

    // Big-endian 32-bit blocks, selected at compile time.
    assert(castle::checksums::basic_fletcher64<false>::calculate(data, sizeof(data)) == 0x3703080D9F686A6CULL);

    checksum.reset();
    assert(checksum.value() == 0U);
    return 0;
}
