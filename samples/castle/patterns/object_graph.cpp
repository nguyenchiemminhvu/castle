#include "sample_support.hpp"

#include "castle/core/status.hpp"
#include "castle/patterns/object_graph.hpp"

#include <stdint.h>

namespace dp = castle::patterns;

// A location service context, wired the way main() would wire it by hand:
//
//   uart_node   --init with--   uart_config
//   hal_adapter --composition-- (uart_node, gpio_led, ubx_parser, nmea_parser, data_pool)
//   ubx_parser  --aggregation-- data_pool
//   nmea_parser --aggregation-- data_pool

// Records the order in which lifecycle hooks run.
struct trace
{
    char steps[16] = {};
    uint32_t count = 0U;

    void push(char step) noexcept
    {
        steps[count++] = step;
    }
};

trace hooks;

// Constant configuration. Default member initializers stand in for device-tree values.
struct uart_config
{
    uint32_t baud = 115200U;
};

struct uart_node
{
    explicit uart_node(const uart_config& config) noexcept
        : baud(config.baud)
    {
    }

    castle::status init() noexcept
    {
        hooks.push('U');
        return castle::status::ok;
    }

    void deinit() noexcept
    {
        hooks.push('u');
    }

    uint32_t baud;
};

struct gpio_led
{
    void init() noexcept
    {
        hooks.push('L');
    }

    void deinit() noexcept
    {
        hooks.push('l');
    }

    bool on = false;
};

// Latest decoded position, shared by both parsers. No lifecycle hooks: the graph skips it.
struct data_pool
{
    int32_t latitude_e7 = 0;
    int32_t longitude_e7 = 0;
};

struct ubx_parser
{
    explicit ubx_parser(data_pool& pool) noexcept
        : pool(pool)
    {
    }

    void decode(int32_t latitude_e7) noexcept
    {
        pool.latitude_e7 = latitude_e7;
    }

    data_pool& pool;
};

struct nmea_parser
{
    explicit nmea_parser(data_pool& pool) noexcept
        : pool(pool)
    {
    }

    void decode(int32_t longitude_e7) noexcept
    {
        pool.longitude_e7 = longitude_e7;
    }

    data_pool& pool;
};

// The HAL adapter is composed of the transport, the LED, the parsers and the pool.
struct hal_adapter
{
    hal_adapter(uart_node& uart, gpio_led& led, ubx_parser& ubx, nmea_parser& nmea, data_pool& pool) noexcept
        : uart(uart)
        , led(led)
        , ubx(ubx)
        , nmea(nmea)
        , pool(pool)
    {
    }

    castle::status init() noexcept
    {
        hooks.push('H');
        led.on = true; // every part is initialized before its owner
        return castle::status::ok;
    }

    void deinit() noexcept
    {
        hooks.push('h');
        led.on = false; // the owner stops before its parts
    }

    void on_frame(int32_t latitude_e7, int32_t longitude_e7) noexcept
    {
        ubx.decode(latitude_e7);
        nmea.decode(longitude_e7);
    }

    uart_node& uart;
    gpio_led& led;
    ubx_parser& ubx;
    nmea_parser& nmea;
    data_pool& pool;
};

// Declaration order is lifetime order: an object is declared after everything it refers to.
using location_context = dp::object_graph<
    uart_config,
    dp::object<uart_node, dp::init_with<uart_config>>,
    gpio_led,
    data_pool,
    dp::object<ubx_parser, dp::uses<data_pool>>,
    dp::object<nmea_parser, dp::uses<data_pool>>,
    dp::object<hal_adapter, dp::owns<uart_node, gpio_led, ubx_parser, nmea_parser, data_pool>>>;

// Static storage, as on Zephyr: every object is constructed and wired here, in one place.
location_context context;

int main()
{
    hal_adapter& hal = context.get<hal_adapter>();
    CASTLE_SAMPLE_CHECK(&hal.pool == &context.get<data_pool>());
    CASTLE_SAMPLE_CHECK(hal.uart.baud == 115200U);

    // Start-up: uart_node, gpio_led, then hal_adapter. Types without init() are skipped.
    CASTLE_SAMPLE_CHECK(context.init() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(context.get<gpio_led>().on);

    hal.on_frame(473977418, 85455939);
    CASTLE_SAMPLE_CHECK(context.get<data_pool>().latitude_e7 == 473977418);
    CASTLE_SAMPLE_CHECK(context.get<data_pool>().longitude_e7 == 85455939);

    // Shutdown: the exact reverse.
    CASTLE_SAMPLE_CHECK(context.deinit() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(!context.get<gpio_led>().on);

    const char expected[] = "ULHhlu";
    for (uint32_t i = 0U; i < 6U; ++i)
    {
        CASTLE_SAMPLE_CHECK(hooks.steps[i] == expected[i]);
    }
    CASTLE_SAMPLE_CHECK(hooks.count == 6U);
    return 0;
}
