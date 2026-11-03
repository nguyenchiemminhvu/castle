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
    uint32_t received = 0U;
    CASTLE_SAMPLE_CHECK(ept.create(dev, "example.service", rpmsg_addr_any, rpmsg_addr_any,
        [&received](message_view const& message) { received += static_cast<uint32_t>(message.size); }) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(ept.is_created());
    CASTLE_SAMPLE_CHECK(ept.source_address() >= reserved_address_count);
    CASTLE_SAMPLE_CHECK(transport.bind(ept.source_address(), 2024U) == castle::status::ok);
    const uint8_t bytes[] = {1U, 2U, 3U};
    CASTLE_SAMPLE_CHECK(ept.try_send(bytes, sizeof(bytes)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(transport.last_packet().destination == 2024U);
    CASTLE_SAMPLE_CHECK(ept.sendto(bytes, sizeof(bytes), 2025U, no_wait) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(transport.last_packet().destination == 2025U);
    CASTLE_SAMPLE_CHECK(transport.inject(ept.source_address(), 2024U, bytes, sizeof(bytes)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(received == 3U);
    tx_buffer tx;
    CASTLE_SAMPLE_CHECK(ept.acquire_tx_buffer(tx) == castle::status::ok);
    tx.data[0] = 0xA5U;
    CASTLE_SAMPLE_CHECK(ept.send_nocopy(tx, 1U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(!tx.active);
    CASTLE_SAMPLE_CHECK(ept.destroy() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(dev.shutdown() == castle::status::ok);
    return 0;
}
