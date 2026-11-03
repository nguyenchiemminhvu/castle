#include "sample_support.hpp"
#include "castle_ext/io/rpmsg/rpmsg.hpp"
#include "castle_ext/io/rpmsg/backends/mock.hpp"

int main()
{
    using namespace castle::io::rpmsg;
    mock::backend transport;
    device<mock::backend> dev(transport);
    CASTLE_SAMPLE_CHECK(dev.initialize() == castle::status::ok);
    endpoint<mock::backend> ept;
    CASTLE_SAMPLE_CHECK(ept.create(dev, "umbrella.service", rpmsg_addr_any, rpmsg_addr_any,
                                   [](message_view const&) {}) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(ept.destroy() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(dev.shutdown() == castle::status::ok);
    return 0;
}
