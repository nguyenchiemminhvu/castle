#include "sample_support.hpp"

#include "castle/callbacks/callback_policy.hpp"

// Scenario: compilation smoke test for callback registration policy configuration.
int main()
{
    // Policy types are compile-time configuration points; including the header is
    // enough to make them available to the registries in a platform build.
    return 0;
}
