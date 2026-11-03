#include <gtest/gtest.h>
#include "castle_ext/io/rpmsg/types.hpp"

using namespace castle::io::rpmsg;

TEST(RpmsgTypes, ProtocolConstantsAndAddressRules)
{
    EXPECT_EQ(rpmsg_name_size, 32U);
    EXPECT_EQ(rpmsg_header_size, 16U);
    EXPECT_EQ(default_max_payload_size, 496U);
    EXPECT_TRUE(is_reserved_address(0U));
    EXPECT_TRUE(is_reserved_address(rpmsg_ns_ept_addr));
    EXPECT_FALSE(is_reserved_address(rpmsg_addr_any));
    EXPECT_FALSE(is_valid_local_address(1023U));
    EXPECT_TRUE(is_valid_local_address(1024U));
    EXPECT_TRUE(is_valid_local_address(rpmsg_addr_any));
    EXPECT_TRUE(is_forever(wait_forever));
    EXPECT_FALSE(is_forever(no_wait));
}

TEST(RpmsgTypes, ConfigurationValidation)
{
    EXPECT_EQ(validate_config(device_config()), castle::status::ok);
    EXPECT_EQ(validate_config(device_config(role::remote, nullptr, 0U, 4U, 512U)), castle::status::ok);
    EXPECT_EQ(validate_config(device_config(static_cast<role>(5U))), castle::status::invalid_config);
    int memory = 0;
    EXPECT_EQ(validate_config(device_config(role::host, &memory, 0U, 4U, 512U)), castle::status::invalid_config);
    EXPECT_EQ(validate_config(device_config(role::host, nullptr, 64U, 4U, 512U)), castle::status::invalid_config);
    EXPECT_EQ(validate_config(device_config(role::host, nullptr, 0U, 0U, 512U)), castle::status::invalid_config);
    EXPECT_EQ(validate_config(device_config(role::host, nullptr, 0U, 4U, 8U)), castle::status::invalid_config);
}

TEST(RpmsgTypes, ConfigurableCapacities)
{
    using tiny = options<2U, 3U, 15U, 64U, 4U, 80U, 48U, false, false, role::remote>;
    EXPECT_EQ(tiny::max_endpoints, 2U);
    EXPECT_EQ(tiny::max_channels, 3U);
    EXPECT_EQ(tiny::max_name_length, 15U);
    EXPECT_EQ(tiny::max_payload_size, 64U);
    EXPECT_FALSE(tiny::auto_bind);
    EXPECT_FALSE(tiny::zero_copy);
    EXPECT_EQ(tiny::default_role, role::remote);
}

