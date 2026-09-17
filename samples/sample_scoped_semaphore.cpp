#include "sample_support.hpp"

#include "castle/sync/scoped_semaphore.hpp"

// Scenario: RAII acquisition of a pool permit that is always returned, even
// on early return paths, without relying on exceptions.
static castle::semaphore<2> pool(2U);

static void use_pool_slot()
{
    castle::scoped_semaphore<2> guard(pool);
    (void)guard;

    // Access one of the up to 2 interchangeable pool slots.
}

int main()
{
    use_pool_slot();
    use_pool_slot();

    CASTLE_SAMPLE_CHECK(pool.count() == 2U);

    return 0;
}
