#include "sample_support.hpp"
#include "castle_ext/io/rpmsg/backends/mock.hpp"
#include "castle_ext/io/rpmsg/rpmsg.hpp"

using namespace castle::io::rpmsg;

int main()
{
    mock::backend transport;
    device<mock::backend> dev(transport);
    uint32_t ns_events = 0U;
    CASTLE_SAMPLE_CHECK(dev.set_name_service_callback([&ns_events](name_service_event const&) { ++ns_events; }) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(dev.initialize() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(transport.announce("log.service", 2400U) == castle::status::ok);
    address remote = rpmsg_addr_any;
    CASTLE_SAMPLE_CHECK(dev.find_service("log.service", remote) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(remote == 2400U && ns_events == 1U);
    CASTLE_SAMPLE_CHECK(dev.endpoint_count() == 0U && dev.service_count() == 1U);
    CASTLE_SAMPLE_CHECK(dev.poll() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(dev.shutdown() == castle::status::ok);
    return 0;
}
