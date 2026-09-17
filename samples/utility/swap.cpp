#include "sample_support.hpp"

#include "castle/utility/swap.hpp"

#include <stdint.h>

int main()
{
    uint32_t a = 1U;
    uint32_t b = 2U;
    castle::swap(a, b);
    CASTLE_SAMPLE_CHECK(a == 2U && b == 1U);
    return 0;
}
