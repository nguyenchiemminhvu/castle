#include <assert.h>
#include <stdint.h>
#include "castle_ext/checksums/fletcher32.hpp"

int main()
{
    const uint8_t data[] = {'1','2','3','4','5','6','7','8','9'};
    castle::checksums::fletcher32 checksum;
    checksum.update(data, 3U);
    checksum.update(data + 3U, sizeof(data) - 3U);
    assert(checksum.value() == 0xDF09D509UL);
    assert(checksum.checksum() == 0xDF09D509UL);
    assert(castle::checksums::fletcher32::calculate(data, sizeof(data)) == 0xDF09D509UL);
    assert(castle::checksums::fletcher32::calculate(static_cast<const void*>(data), sizeof(data)) == 0xDF09D509UL);

    // Big-endian 16-bit blocks, selected at compile time.
    assert(castle::checksums::basic_fletcher32<false>::calculate(data, sizeof(data)) == 0x09DF09D5UL);

    checksum.reset();
    assert(checksum.value() == 0U);
    return 0;
}
