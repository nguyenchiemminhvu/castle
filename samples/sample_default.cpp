#include "sample_support.hpp"

#include "castle/core/compiler_variants/default.hpp"

// Scenario: cross-toolchain inclusion test for a compiler-specific variant.
int main()
{
    // Compiler variant headers are selected by compiler.h. Keep direct inclusion
    // as a compile smoke test for cross-toolchain CI.
    return 0;
}
