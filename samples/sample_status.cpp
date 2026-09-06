#include "sample_support.h"

#include "castle/error/status.h"

// Scenario: checking non-throwing operation results used throughout Castle containers.
int main()
{
    static_assert(castle::status::ok != castle::status::full, "status values must differ");
    CASTLE_SAMPLE_CHECK(castle::status::invalid_argument != castle::status::out_of_range);
    return 0;
}
