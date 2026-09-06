/**
 * Castle 2.0 sample: <castle/math/random.h>
 *
 * Scenario: deterministic pseudo-random backoff for retry scheduling. The PRNG
 * has explicit seed/state and does not depend on libc rand/srand or a heap.
 */
#include "sample_support.h"
#include "castle/math/random.h"

#include <stdint.h>

int main()
{
    castle::math::random generator(0x12345678U);

    const uint32_t first = generator.next();
    const uint32_t second = generator();
    const uint32_t retry_delay = generator.uniform(100U);
    const uint32_t bounded = generator.range(10U, 20U);

    CASTLE_SAMPLE_CHECK(first != second);
    CASTLE_SAMPLE_CHECK(retry_delay < 100U);
    CASTLE_SAMPLE_CHECK(bounded >= 10U && bounded <= 20U);

    generator.discard(4U);
    CASTLE_SAMPLE_CHECK(generator.min() == 0U);
    CASTLE_SAMPLE_CHECK(generator.max() == UINT32_MAX);
    return 0;
}
