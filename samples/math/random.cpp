#include "sample_support.hpp"
#include "castle/math/random.hpp"

#include <stdint.h>

int main()
{
    castle::math::random a(0x12345678U);
    castle::math::random b(0x12345678U);
    castle::math::random c;
    castle::math::random d;
    castle::math::random skip(0x12345678U);

    const uint32_t first_a = a.next();
    const uint32_t first_b = b.next();
    const uint32_t second_a = a.next();
    const uint32_t second_b = b();
    const uint32_t first_c = c.next();
    const uint32_t first_d = d.next();

    CASTLE_SAMPLE_CHECK(first_a == first_b);
    CASTLE_SAMPLE_CHECK(second_a == second_b);
    CASTLE_SAMPLE_CHECK(first_c == first_d);

    skip.discard(2U);
    CASTLE_SAMPLE_CHECK(a.next() == skip.next());

    for (uint32_t i = 0U; i < 16U; ++i)
    {
        const uint32_t bounded = c.uniform(100U);
        const uint32_t ranged = c.range(10U, 20U);
        CASTLE_SAMPLE_CHECK(bounded < 100U);
        CASTLE_SAMPLE_CHECK(ranged >= 10U && ranged <= 20U);
    }

    CASTLE_SAMPLE_CHECK(castle::math::random::min() == 0U);
    CASTLE_SAMPLE_CHECK(castle::math::random::max() == UINT32_MAX);
    return 0;
}
