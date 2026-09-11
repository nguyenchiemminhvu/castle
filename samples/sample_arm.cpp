#include "sample_support.h"

#include "castle/core/compiler_variants/arm.h"

// Scenario: cross-toolchain inclusion test for a compiler-specific variant.
int main()
{
    // Compiler variant headers are selected by compiler.h. Keep direct inclusion
    // as a compile smoke test for cross-toolchain CI.
    return 0;
}
