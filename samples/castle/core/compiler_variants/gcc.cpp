#include "sample_support.hpp"

#include "castle/core/compiler_variants/gcc.hpp"

#include <stdint.h>

namespace
{

struct CASTLE_PACKED_ATTR PackedPair
{
    uint8_t first;
    uint16_t second;
};

CASTLE_INLINE int fast_sum(const int* CASTLE_RESTRICT values)
{
    if (CASTLE_LIKELY(values != nullptr))
    {
        return values[0] + values[1];
    }
    return 0;
}

} // namespace

int main()
{
    const int values[2] = {1, 2};
    CASTLE_SAMPLE_CHECK(CASTLE_COMPILER_GCC == 1);
    CASTLE_SAMPLE_CHECK(sizeof(PackedPair) == 3U);
    CASTLE_SAMPLE_CHECK(fast_sum(values) == 3);
    CASTLE_SAMPLE_CHECK(!CASTLE_UNLIKELY(false));
    CASTLE_CPU_RELAX();
    if (false)
    {
        CASTLE_UNREACHABLE();
    }
    return 0;
}
