#include "sample_support.h"

#include "castle/memory/addressof.h"

// Scenario: taking the true address of a low-level object without overloaded operator& interference.
int main()
{
    uint32_t value = 0U;
    CASTLE_SAMPLE_CHECK(castle::memory::addressof(value) == &value);
    return 0;
}
