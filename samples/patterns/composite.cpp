#include "sample_support.hpp"

#include "castle/patterns/composite.hpp"

#include <stdint.h>
#include <stdio.h>

namespace dp = castle::patterns;

// Every component performs the same task (shutdown) but has its own, unrelated type.
// No common base class is needed.
uint32_t shutdown_count = 0U;

// ----- Application layer -----
struct hmi_app
{
    void shutdown() noexcept
    {
        printf("[app]      hmi_app shutdown\n");
        ++shutdown_count;
    }
};

// ----- Service layer -----
struct dtc_service
{
    void shutdown() noexcept
    {
        printf("[service]  dtc_service shutdown\n");
        ++shutdown_count;
    }
};

struct diag_service
{
    void shutdown() noexcept
    {
        printf("[service]  diag_service shutdown\n");
        ++shutdown_count;
    }
};

// ----- HAL layer -----
struct gpio_driver
{
    void shutdown() noexcept
    {
        printf("[hal]      gpio_driver shutdown\n");
        ++shutdown_count;
    }
};

struct can_driver
{
    void shutdown() noexcept
    {
        printf("[hal/bus]  can_driver shutdown\n");
        ++shutdown_count;
    }
};

struct spi_driver
{
    void shutdown() noexcept
    {
        printf("[hal/bus]  spi_driver shutdown\n");
        ++shutdown_count;
    }
};

int main()
{
    // The main service owns every instance.
    hmi_app hmi;
    dtc_service dtc;
    diag_service diag;
    gpio_driver gpio;
    can_driver can;
    spi_driver spi;

    // Group the instances layer by layer. Groups only refer to the instances.
    auto application = dp::tie_composite(hmi);
    auto services = dp::tie_composite(dtc, diag);
    auto bus_drivers = dp::tie_composite(can, spi);
    auto hal = dp::tie_composite(gpio, bus_drivers); // a group can contain another group

    // One system object wraps all layers.
    auto system = dp::tie_composite(application, services, hal);

    // A single call reaches every component, top-down and left-to-right.
    system.for_each([](auto& component)
    {
        component.shutdown();
    });

    CASTLE_SAMPLE_CHECK(shutdown_count == 6U);
    return 0;
}
