#include <gtest/gtest.h>

#include "castle/core/traits.hpp"
#include "castle_ext/io/can/types.hpp"

namespace
{

using namespace castle::io::can;

static_assert(castle::meta::is_trivially_copyable<can_config>::value, "can_config must be trivially copyable");
static_assert(castle::meta::is_trivially_copyable<bus_status>::value, "bus_status must be trivially copyable");
static_assert(castle::meta::is_trivially_copyable<timeout_type>::value, "timeout_type must be trivially copyable");
static_assert(castle::meta::is_trivially_destructible<can_config>::value, "can_config must be trivially destructible");
static_assert(castle::meta::is_trivially_destructible<bus_status>::value, "bus_status must be trivially destructible");
static_assert(castle::meta::is_standard_layout<can_config>::value, "can_config must be standard layout");
static_assert(castle::meta::is_standard_layout<bus_status>::value, "bus_status must be standard layout");

static_assert(castle::meta::is_same<timeout_type, castle::chrono::milliseconds>::value,
              "timeout_type is a millisecond duration");

// constexpr use
static_assert(no_wait.count() == 0, "no_wait is zero");
static_assert(is_forever(wait_forever), "wait_forever is the sentinel");
static_assert(!is_forever(no_wait), "no_wait is not forever");
static_assert(is_operational(bus_state::error_active), "error_active is operational");
static_assert(!is_operational(bus_state::bus_off), "bus_off is not operational");
constexpr can_config constexpr_config(bitrate_500k, can_mode::loopback);
static_assert(constexpr_config.bitrate == 500000U, "constexpr config bitrate");
static_assert(constexpr_config.mode == can_mode::loopback, "constexpr config mode");
constexpr bus_status constexpr_status{};
static_assert(constexpr_status.state == bus_state::stopped, "constexpr status default");

TEST(CanTypes, NoWaitIsZeroMilliseconds)
{
    EXPECT_EQ(no_wait.count(), 0);
    EXPECT_FALSE(is_forever(no_wait));
    EXPECT_EQ(no_wait, timeout_type(0));
}

TEST(CanTypes, WaitForeverIsLargestRepresentableTimeout)
{
    EXPECT_EQ(wait_forever, timeout_type::max());
    EXPECT_GT(wait_forever, timeout_type(1000000));
    EXPECT_GT(wait_forever, no_wait);
}

TEST(CanTypes, IsForeverOnlyForTheSentinel)
{
    EXPECT_TRUE(is_forever(wait_forever));
    EXPECT_TRUE(is_forever(timeout_type::max()));
    EXPECT_TRUE(is_forever(timeout_type(wait_forever.count())));
    EXPECT_FALSE(is_forever(timeout_type(wait_forever.count() - 1)));
    EXPECT_FALSE(is_forever(no_wait));
    EXPECT_FALSE(is_forever(timeout_type(1)));
    EXPECT_FALSE(is_forever(timeout_type(-1)));
    EXPECT_FALSE(is_forever(timeout_type::min()));
}

TEST(CanTypes, TimeoutTypeHasMillisecondSemantics)
{
    const timeout_type one_second = castle::chrono::duration_cast<timeout_type>(castle::chrono::seconds(1));
    EXPECT_EQ(one_second.count(), 1000);
    EXPECT_EQ(timeout_type(250).count(), 250);
    EXPECT_LT(timeout_type(-1), no_wait);
    EXPECT_TRUE(timeout_type(-5).count() < 0);
}

TEST(CanTypes, BitrateConstantsHaveCanonicalValues)
{
    EXPECT_EQ(bitrate_10k, 10000U);
    EXPECT_EQ(bitrate_20k, 20000U);
    EXPECT_EQ(bitrate_50k, 50000U);
    EXPECT_EQ(bitrate_100k, 100000U);
    EXPECT_EQ(bitrate_125k, 125000U);
    EXPECT_EQ(bitrate_250k, 250000U);
    EXPECT_EQ(bitrate_500k, 500000U);
    EXPECT_EQ(bitrate_800k, 800000U);
    EXPECT_EQ(bitrate_1m, 1000000U);
    EXPECT_EQ(max_bitrate, 1000000U);
}

TEST(CanTypes, BitrateConstantsAreStrictlyAscendingAndBoundedByMax)
{
    const uint32_t rates[] = {bitrate_10k,  bitrate_20k,  bitrate_50k,  bitrate_100k, bitrate_125k,
                              bitrate_250k, bitrate_500k, bitrate_800k, bitrate_1m};
    for (size_t i = 1U; i < sizeof(rates) / sizeof(rates[0]); ++i)
    {
        EXPECT_LT(rates[i - 1U], rates[i]) << i;
    }
    for (const uint32_t rate : rates)
    {
        EXPECT_LE(rate, max_bitrate);
        EXPECT_EQ(validate_config(can_config(rate)), castle::status::ok) << rate;
    }
    EXPECT_EQ(max_bitrate, bitrate_1m);
}

TEST(CanTypes, CanModeEnumeratorValuesAreStable)
{
    EXPECT_EQ(static_cast<uint8_t>(can_mode::normal), 0U);
    EXPECT_EQ(static_cast<uint8_t>(can_mode::loopback), 1U);
    EXPECT_EQ(static_cast<uint8_t>(can_mode::listen_only), 2U);
}

TEST(CanTypes, CanConfigDefaultsKeepDriverBitrateAndNormalMode)
{
    const can_config cfg;
    EXPECT_EQ(cfg.bitrate, 0U);
    EXPECT_EQ(cfg.mode, can_mode::normal);
    EXPECT_EQ(validate_config(cfg), castle::status::ok);
}

TEST(CanTypes, CanConfigStoresArguments)
{
    const can_config only_rate(bitrate_250k);
    EXPECT_EQ(only_rate.bitrate, 250000U);
    EXPECT_EQ(only_rate.mode, can_mode::normal);

    const can_config both(bitrate_125k, can_mode::listen_only);
    EXPECT_EQ(both.bitrate, 125000U);
    EXPECT_EQ(both.mode, can_mode::listen_only);
}

TEST(CanTypes, ValidateConfigBitrateBoundaries)
{
    EXPECT_EQ(validate_config(can_config(0U)), castle::status::ok);
    EXPECT_EQ(validate_config(can_config(1U)), castle::status::ok);
    EXPECT_EQ(validate_config(can_config(1000000U)), castle::status::ok);
    EXPECT_EQ(validate_config(can_config(1000001U)), castle::status::invalid_config);
    EXPECT_EQ(validate_config(can_config(0xFFFFFFFFU)), castle::status::invalid_config);
}

TEST(CanTypes, ValidateConfigAcceptsEveryKnownMode)
{
    EXPECT_EQ(validate_config(can_config(bitrate_500k, can_mode::normal)), castle::status::ok);
    EXPECT_EQ(validate_config(can_config(bitrate_500k, can_mode::loopback)), castle::status::ok);
    EXPECT_EQ(validate_config(can_config(bitrate_500k, can_mode::listen_only)), castle::status::ok);
}

TEST(CanTypes, ValidateConfigRejectsUnknownMode)
{
    EXPECT_EQ(validate_config(can_config(bitrate_500k, static_cast<can_mode>(3U))), castle::status::invalid_config);
    EXPECT_EQ(validate_config(can_config(0U, static_cast<can_mode>(0xFFU))), castle::status::invalid_config);
}

TEST(CanTypes, ValidateConfigRejectsBothFaultsTogether)
{
    EXPECT_EQ(validate_config(can_config(2000000U, static_cast<can_mode>(9U))), castle::status::invalid_config);
}

TEST(CanTypes, BusStateEnumeratorValuesAreStable)
{
    EXPECT_EQ(static_cast<uint8_t>(bus_state::stopped), 0U);
    EXPECT_EQ(static_cast<uint8_t>(bus_state::error_active), 1U);
    EXPECT_EQ(static_cast<uint8_t>(bus_state::error_warning), 2U);
    EXPECT_EQ(static_cast<uint8_t>(bus_state::error_passive), 3U);
    EXPECT_EQ(static_cast<uint8_t>(bus_state::bus_off), 4U);
}

TEST(CanTypes, BusStatusDefaultsToStoppedWithZeroCounters)
{
    const bus_status status;
    EXPECT_EQ(status.state, bus_state::stopped);
    EXPECT_EQ(status.tx_error_counter, 0U);
    EXPECT_EQ(status.rx_error_counter, 0U);
    EXPECT_EQ(status.rx_overruns, 0U);
    EXPECT_EQ(status.tx_failures, 0U);
}

TEST(CanTypes, BusStatusIsPlainAssignableData)
{
    bus_status status;
    status.state = bus_state::error_passive;
    status.tx_error_counter = 130U;
    status.rx_error_counter = 7U;
    status.rx_overruns = 3U;
    status.tx_failures = 9U;
    const bus_status copy = status;
    EXPECT_EQ(copy.state, bus_state::error_passive);
    EXPECT_EQ(copy.tx_error_counter, 130U);
    EXPECT_EQ(copy.rx_error_counter, 7U);
    EXPECT_EQ(copy.rx_overruns, 3U);
    EXPECT_EQ(copy.tx_failures, 9U);
}

TEST(CanTypes, IsOperationalForAllFiveStates)
{
    EXPECT_FALSE(is_operational(bus_state::stopped));
    EXPECT_TRUE(is_operational(bus_state::error_active));
    EXPECT_TRUE(is_operational(bus_state::error_warning));
    EXPECT_FALSE(is_operational(bus_state::error_passive));
    EXPECT_FALSE(is_operational(bus_state::bus_off));
}

TEST(CanTypes, IsOperationalRejectsOutOfRangeState)
{
    EXPECT_FALSE(is_operational(static_cast<bus_state>(5U)));
    EXPECT_FALSE(is_operational(static_cast<bus_state>(0xFFU)));
}

} // namespace
