#include "sample_support.hpp"
#include "castle_ext/io/rpmsg/backends/mock.hpp"
#include "castle_ext/io/rpmsg/rpmsg.hpp"

using namespace castle::io::rpmsg;

int main()
{
    mock::backend transport;
    device<mock::backend> dev(transport);
    CASTLE_SAMPLE_CHECK(dev.initialize() == castle::status::ok);
    endpoint<mock::backend> ept;
    uint32_t rx_count = 0U;
    CASTLE_SAMPLE_CHECK(ept.create(dev, "mock.service", rpmsg_addr_any, rpmsg_addr_any,
        [&rx_count](message_view const& msg) { rx_count += static_cast<uint32_t>(msg.size); }) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(transport.bind(ept.source_address(), 3100U) == castle::status::ok);
    const uint8_t payload[] = {7U, 8U};
    CASTLE_SAMPLE_CHECK(transport.inject(ept.source_address(), 3100U, payload, sizeof(payload)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(rx_count == 2U);
    transport.fail_next_send(castle::status::full);
    CASTLE_SAMPLE_CHECK(ept.try_send(payload, sizeof(payload)) == castle::status::full);
    CASTLE_SAMPLE_CHECK(ept.destroy() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(dev.shutdown() == castle::status::ok);
    return 0;
}
