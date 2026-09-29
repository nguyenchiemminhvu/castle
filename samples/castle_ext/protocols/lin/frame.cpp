#include <assert.h>
#include <stdint.h>

#include "castle_ext/protocols/lin/frame.hpp"

int main()
{
    using namespace castle::protocols::lin;
    header request{};
    assert(castle::succeeded(make_header(0x12U, request)));
    assert(request.valid());
    assert(is_valid_protected_identifier(request.protected_identifier));
    assert(LIN_SYNC_BYTE == 0x55U);
    assert(LIN_MAX_DATA_LENGTH == 8U);
    assert(!is_valid_identifier(LIN_RESERVED_IDENTIFIER_FIRST));

    header decoded{};
    assert(castle::succeeded(decode_header(request.protected_identifier, decoded)));
    assert(decoded.identifier == request.identifier);

    const uint8_t old_pid = decoded.protected_identifier;
    assert(decode_header(0x12U, decoded) == castle::status::invalid_argument);
    assert(decoded.protected_identifier == old_pid);
    assert(calculate_protected_identifier(LIN_RESERVED_IDENTIFIER_FIRST)
        == LIN_INVALID_PROTECTED_IDENTIFIER);
    return 0;
}
