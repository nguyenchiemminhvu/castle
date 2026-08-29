#include "castle/logging/log_builder.h"

#include <cstdint>
#include <iostream>
#include <string_view>

using namespace castle::logging;

// ============================================================================
// Console sinks
// ============================================================================

void cout_sink(void* /*context*/, std::string_view record) noexcept
{
    std::cout << record << '\n';
}

void cerr_sink(void* /*context*/, std::string_view record) noexcept
{
    std::cerr << record << '\n';
}

// ============================================================================
// Fake embedded system timer
// ============================================================================

struct system_timer
{
    std::uint32_t tick_ms{0U};

    std::uint32_t milliseconds() const noexcept
    {
        return tick_ms;
    }
};

std::uint32_t timer_callback(void* context) noexcept
{
    const auto& timer = *static_cast<const system_timer*>(context);

    return timer.milliseconds();
}

// ============================================================================
// Application
// ============================================================================

int main()
{
    // ------------------------------------------------------------------------
    // System timer
    //
    // In a real MCU this could be:
    //
    //     SysTick
    //     GPT
    //     AUTOSAR OsCounter
    //     HAL_GetTick()
    // ------------------------------------------------------------------------

    system_timer timer{1234U};

    // ------------------------------------------------------------------------
    // Main logger
    // ------------------------------------------------------------------------

    log_builder<256U> log{
        {
            &cout_sink,
            nullptr
        },
        {
            &timer_callback,
            &timer
        }
    };

    // ------------------------------------------------------------------------
    // Configure logger
    // ------------------------------------------------------------------------

    log.set_min_level(log_level::debug);

    // ------------------------------------------------------------------------
    // Application data
    // ------------------------------------------------------------------------

    const int satellites = 8;
    const bool has_fix = true;

    // ------------------------------------------------------------------------
    // Log messages
    // ------------------------------------------------------------------------

    log.info(
        "Satellites in view: ",
        satellites,
        " (fix=",
        has_fix,
        ")"
    );

    log.debug(
        "This is a debug message containing a number: ",
        42
    );

    log.warning(
        "UART buffer is filling up! Current load: ",
        85,
        "%"
    );

    log.error(
        "Hardware fault detected!"
    );

    // =========================================================================
    // Local logger with different configuration
    // =========================================================================

    std::cout << "\n--- Testing local logger instance ---\n";

    // ------------------------------------------------------------------------
    // A completely independent logger.
    //
    // This logger has:
    //
    //     - its own buffer capacity
    //     - its own sink
    //     - its own log level
    //
    // It has no relationship with `log`.
    // ------------------------------------------------------------------------

    log_builder<128U> local_logger{
        {
            &cerr_sink,
            nullptr
        }
    };

    local_logger.set_min_level(
        log_level::warning
    );

    // ------------------------------------------------------------------------
    // Filtered out:
    //
    // info < warning
    // ------------------------------------------------------------------------

    local_logger.info(
        "This info log will NOT be printed."
    );

    // ------------------------------------------------------------------------
    // Printed:
    //
    // warning >= warning
    // ------------------------------------------------------------------------

    local_logger.warning(
        "Local logger alert: Temperature threshold exceeded!"
    );

    // ------------------------------------------------------------------------
    // Error is also printed.
    // ------------------------------------------------------------------------

    local_logger.error(
        "Local logger hardware fault!"
    );

    // =========================================================================
    // Multiple loggers with different buffer sizes
    // =========================================================================

    std::cout << "\n--- Multiple logger instances ---\n";

    log_builder<64U> small_logger{
        {
            &cout_sink,
            nullptr
        }
    };

    log_builder<512U> large_logger{
        {
            &cout_sink,
            nullptr
        }
    };

    small_logger.info(
        "Small diagnostic message"
    );

    large_logger.info(
        "Large diagnostic message: ",
        "this logger has a larger compile-time buffer capacity."
    );

    log_builder<32U> tiny_logger{
        {
            &cout_sink,
            nullptr
        }
    };

    // ------------------------------------------------------------------------
    // Overflow demonstration for tiny logger (buffer too small)
    // ------------------------------------------------------------------------
    tiny_logger.info(
        "This is a long string to fill the tiny logger's buffer.\n"
    );

    return 0;
}
