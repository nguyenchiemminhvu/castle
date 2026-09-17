#include "sample_support.hpp"

#include "castle/core/compiler_variants/default.hpp"

#include <stdint.h>

namespace
{

CASTLE_INLINE int fallback_value()
{
    return 3;
}

struct CASTLE_PACKED_ATTR PackedPair
{
    uint8_t first;
    uint16_t second;
};

} // namespace

int main()
{
    CASTLE_SAMPLE_CHECK(CASTLE_COMPILER_UNKNOWN == 1);
    CASTLE_SAMPLE_CHECK(fallback_value() == 3);
    CASTLE_SAMPLE_CHECK(CASTLE_LIKELY(true));
    CASTLE_SAMPLE_CHECK(!CASTLE_UNLIKELY(false));
    CASTLE_CPU_RELAX();
    if (false)
    {
        CASTLE_UNREACHABLE();
    }
    CASTLE_SAMPLE_CHECK(sizeof(PackedPair) >= 3U);
    return 0;
}
