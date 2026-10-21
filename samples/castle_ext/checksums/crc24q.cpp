#include <assert.h>
#include <stdint.h>
#include "castle_ext/checksums/crc24q.hpp"

int main()
{
    const uint8_t data[] = {'1','2','3','4','5','6','7','8','9'};

    // Streaming: feed the message in chunks, read the value at any time.
    castle::checksums::crc24q crc;
    crc.update(data, 4U);
    crc.update(static_cast<const void*>(data + 4U), 5U);
    assert(crc.value() == 0xCDE703U);
    assert(crc.checksum() == 0xCDE703U);

    // One-shot calculation; the result occupies the low 24 bits.
    assert(castle::checksums::crc24q::calculate(data, sizeof(data)) == 0xCDE703U);
    assert(castle::checksums::crc24q::calculate(static_cast<const void*>(data), sizeof(data)) == 0xCDE703U);
    assert((castle::checksums::crc24q::calculate(data, sizeof(data)) >> 24U) == 0U);

    // A null pointer is ignored; an empty message yields the initial value 0.
    assert(castle::checksums::crc24q::calculate(static_cast<const uint8_t*>(nullptr), 4U) == 0U);

    // Appending the CRC (most significant byte first) leaves a zero remainder.
    uint8_t protected_message[12U] = {'1','2','3','4','5','6','7','8','9', 0xCDU, 0xE7U, 0x03U};
    assert(castle::checksums::crc24q::calculate(protected_message, sizeof(protected_message)) == 0U);

    crc.reset();
    assert(crc.value() == 0U);

    // The shared engine accepts other Rocksoft parameter sets, e.g. CRC-24/OPENPGP.
    using crc24_openpgp = castle::checksums::detail::basic_crc<uint32_t,
                                                               static_cast<uint32_t>(0x864CFBU),
                                                               static_cast<uint32_t>(0xB704CEU),
                                                               static_cast<uint32_t>(0x000000U),
                                                               false,
                                                               false,
                                                               24U>;
    assert(crc24_openpgp::calculate(data, sizeof(data)) == 0x21CF02U);
    return 0;
}
