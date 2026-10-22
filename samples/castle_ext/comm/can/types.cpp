#include "sample_support.hpp"

#include "castle_ext/comm/can/types.hpp"

using namespace castle::comm::can;

int main()
{
    // Timeouts are plain millisecond durations.
    static_assert(no_wait.count() == 0, "no_wait never blocks");
    static_assert(is_forever(wait_forever), "wait_forever is the blocking sentinel");
    static_assert(!is_forever(no_wait), "a zero timeout is not forever");
    static_assert(!is_forever(timeout_type(1000)), "finite timeouts are not forever");

    CASTLE_SAMPLE_CHECK(!is_forever(timeout_type(250)));
    CASTLE_SAMPLE_CHECK(is_forever(timeout_type::max()));

    // Bit-rate constants cover the classic CAN range.
    static_assert(bitrate_10k < bitrate_20k && bitrate_20k < bitrate_50k && bitrate_50k < bitrate_100k
                      && bitrate_100k < bitrate_125k && bitrate_125k < bitrate_250k
                      && bitrate_250k < bitrate_500k && bitrate_500k < bitrate_800k && bitrate_800k < bitrate_1m,
                  "bit rates ascend");
    CASTLE_SAMPLE_CHECK(bitrate_125k == 125000U);
    CASTLE_SAMPLE_CHECK(bitrate_500k == 500000U);
    CASTLE_SAMPLE_CHECK(max_bitrate == bitrate_1m);

    // Default configuration keeps the driver bit rate (0) in normal mode.
    const can_config defaults;
    CASTLE_SAMPLE_CHECK(defaults.bitrate == 0U);
    CASTLE_SAMPLE_CHECK(defaults.mode == can_mode::normal);
    CASTLE_SAMPLE_CHECK(validate_config(defaults) == castle::status::ok);

    // Typical vehicle-bus configuration, and a self-test configuration.
    const can_config vehicle(bitrate_500k);
    CASTLE_SAMPLE_CHECK(vehicle.bitrate == bitrate_500k);
    CASTLE_SAMPLE_CHECK(validate_config(vehicle) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(validate_config(can_config(bitrate_250k, can_mode::loopback)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(validate_config(can_config(bitrate_1m, can_mode::listen_only)) == castle::status::ok);

    // Rejected configurations report invalid_config without touching hardware.
    CASTLE_SAMPLE_CHECK(validate_config(can_config(max_bitrate + 1U)) == castle::status::invalid_config);
    CASTLE_SAMPLE_CHECK(validate_config(can_config(bitrate_500k, static_cast<can_mode>(9U)))
                        == castle::status::invalid_config);

    // Bus status starts out stopped with all counters at zero.
    const bus_status idle;
    CASTLE_SAMPLE_CHECK(idle.state == bus_state::stopped);
    CASTLE_SAMPLE_CHECK(idle.tx_error_counter == 0U && idle.rx_error_counter == 0U);
    CASTLE_SAMPLE_CHECK(idle.rx_overruns == 0U && idle.tx_failures == 0U);

    // is_operational(): the node can still transmit normally in error_active and error_warning only.
    CASTLE_SAMPLE_CHECK(is_operational(bus_state::error_active));
    CASTLE_SAMPLE_CHECK(is_operational(bus_state::error_warning));
    CASTLE_SAMPLE_CHECK(!is_operational(bus_state::stopped));
    CASTLE_SAMPLE_CHECK(!is_operational(bus_state::error_passive));
    CASTLE_SAMPLE_CHECK(!is_operational(bus_state::bus_off));

    return 0;
}
