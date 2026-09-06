#include "sample_support.h"

#include "castle/chrono/clock_variants/clang_clock_variant.h"

// Scenario: ensuring the target-specific clock variant header remains independently includable.
int main()
{
    // Platform clock variant headers provide the compiler/target-specific
    // implementation selected by clocks.h; this sample is intentionally a
    // compile smoke test rather than a host timing test.
    return 0;
}
