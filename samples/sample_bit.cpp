/**
 * Castle 2.0 sample: <castle/bit/bit.h>
 *
 * Scenario: an MCU status register is decoded and updated using the bit facade.
 * The same pattern is typical for GPIO, UART, DMA, and timer control registers.
 */
#include "sample_support.h"
#include "castle/bit/bit.h"

#include <stdint.h>

namespace
{
constexpr uint32_t kRxReady = (1U << 0U);
constexpr uint32_t kTxReady = (1U << 1U);
constexpr uint32_t kOverrun = (1U << 7U);

void demo_status_register()
{
    uint32_t status = kRxReady | kOverrun;

    CASTLE_SAMPLE_CHECK(castle::bit::test(status, kRxReady));
    CASTLE_SAMPLE_CHECK(!castle::bit::test(status, kTxReady));

    castle::bit::set(status, kTxReady);
    CASTLE_SAMPLE_CHECK(castle::bit::test(status, kTxReady));
}
} // namespace

int main()
{
    demo_status_register();
    return 0;
}
