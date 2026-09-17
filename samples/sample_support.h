#ifndef CASTLE_SAMPLE_SUPPORT_H
#define CASTLE_SAMPLE_SUPPORT_H

#include <stdint.h>

namespace castle_sample
{
inline volatile uint32_t failure_line = 0U;

[[noreturn]] inline void fail(uint32_t line) noexcept
{
    failure_line = line;
    for (;;)
    {
        /* Bare-metal fail-stop: replace with board watchdog/error LED hook. */
    }
}
} // namespace castle_sample

#define CASTLE_SAMPLE_CHECK(condition) \
    do \
    { \
        if (!(condition)) \
        { \
            ::castle_sample::fail(static_cast<uint32_t>(__LINE__)); \
        } \
    } while (false)

#endif // CASTLE_SAMPLE_SUPPORT_H
