#include "sample_support.h"

#include "castle/core/compiler.h"

// Scenario: verifying that the Castle compiler abstraction can be included by a board build.
int main()
{
    static_assert(sizeof(int) > 0U, "host compiler sanity");
    return 0;
}
