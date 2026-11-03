#include "sample_support.hpp"
#include "castle_ext/io/rpmsg/types.hpp"

using namespace castle::io::rpmsg;
using tiny_options = options<4U, 3U, 20U, 128U, 8U, 256U>;

int main()
{
    static_assert(rpmsg_header_size == 16U, "wire header size");
    static_assert(default_max_payload_size == 496U, "512-byte buffer payload");
    static_assert(tiny_options::max_endpoints == 4U, "fixed capacity");
    CASTLE_SAMPLE_CHECK(is_reserved_address(35U));
    CASTLE_SAMPLE_CHECK(!is_valid_local_address(35U));
    CASTLE_SAMPLE_CHECK(is_valid_local_address(1024U));
    CASTLE_SAMPLE_CHECK(is_valid_local_address(rpmsg_addr_any));
    CASTLE_SAMPLE_CHECK(is_forever(wait_forever));
    CASTLE_SAMPLE_CHECK(validate_config(device_config(role::remote)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(validate_config(device_config(static_cast<role>(3U))) == castle::status::invalid_config);
    return 0;
}
