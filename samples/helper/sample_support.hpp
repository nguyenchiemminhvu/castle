#ifndef CASTLE_SAMPLE_SUPPORT_HPP
#define CASTLE_SAMPLE_SUPPORT_HPP

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

namespace castle_sample
{
inline volatile uint32_t failure_line = 0U;

[[noreturn]] inline void fail(uint32_t line) noexcept
{
    failure_line = line;
    fprintf(stderr, "CASTLE_SAMPLE_CHECK failed at line %u\n", static_cast<unsigned>(line));
    abort();
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

#endif // CASTLE_SAMPLE_SUPPORT_HPP
