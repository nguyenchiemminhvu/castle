#include <assert.h>
#include <stdint.h>

#include "castle/container/array_view.hpp"
#include "castle_ext/protocols/lin/checksum.hpp"

int main()
{
    using namespace castle::protocols::lin;
    const uint8_t payload[3U] = {0x12U, 0x34U, 0x56U};
    frame value{};
    assert(castle::succeeded(make_frame(0x12U, payload, 3U,
        checksum_type::enhanced, value)));
    assert(value.valid());
    assert(validate_checksum(value));

    uint8_t checksum = 0U;
    assert(castle::succeeded(calculate_checksum(value, checksum)));
    assert(checksum == value.checksum);

    const castle::container::array_view<const uint8_t> view(payload, 3U);
    frame viewed{};
    assert(castle::succeeded(make_frame(0x12U, view, checksum_type::enhanced, viewed)));
    assert(validate_checksum(viewed));
    assert(castle::succeeded(calculate_checksum(0x12U, view, checksum_type::enhanced, checksum)));
    assert(checksum == viewed.checksum);

    value.data[0U] ^= 0x01U;
    assert(!validate_checksum(value));

    const uint8_t diagnostic[LIN_MAX_DATA_LENGTH] = {};
    frame diag{};
    assert(castle::succeeded(make_frame(LIN_DIAGNOSTIC_REQUEST_IDENTIFIER,
        diagnostic, LIN_MAX_DATA_LENGTH, checksum_type::enhanced, diag)));
    assert(diag.checksum_mode == checksum_type::classic);
    assert(validate_checksum(diag));
    return 0;
}
