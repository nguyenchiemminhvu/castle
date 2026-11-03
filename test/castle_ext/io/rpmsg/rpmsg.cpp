#include <gtest/gtest.h>
#include "castle_ext/io/rpmsg/rpmsg.hpp"
#include "castle_ext/io/rpmsg/backends/mock.hpp"

TEST(RpmsgUmbrella, ExposesUsableCommonFacade)
{
    using namespace castle::io::rpmsg;
    mock::backend backend;
    device<mock::backend> dev(backend);
    ASSERT_EQ(dev.initialize(), castle::status::ok);
    endpoint<mock::backend> ept;
    ASSERT_EQ(ept.create(dev, "umbrella", rpmsg_addr_any, 3333U, [](message_view const&) {}), castle::status::ok);
    EXPECT_EQ(ept.destination_address(), 3333U);
    EXPECT_EQ(ept.try_send(nullptr, 0U), castle::status::ok);
    EXPECT_EQ(ept.destroy(), castle::status::ok);
    EXPECT_EQ(dev.shutdown(), castle::status::ok);
}

