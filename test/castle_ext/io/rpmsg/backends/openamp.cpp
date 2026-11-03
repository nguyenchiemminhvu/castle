#include <gtest/gtest.h>
#include "castle_ext/io/rpmsg/backends/openamp.hpp"
#include "castle_ext/io/rpmsg/rpmsg.hpp"

TEST(RpmsgOpenAmpBackend, HeaderIsOptionalWithoutSdkMacro)
{
    EXPECT_EQ(castle::io::rpmsg::rpmsg_header_size, 16U);
    EXPECT_EQ(castle::io::rpmsg::default_buffer_size, 512U);
#if defined(CASTLE_RPMSG_ENABLE_OPENAMP)
    SUCCEED();
#else
    SUCCEED();
#endif
}

