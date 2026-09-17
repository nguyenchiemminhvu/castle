#include "sample_support.hpp"

#include "castle/core/error_handler.hpp"

// Scenario: board-specific fail-stop policy integration.
int main()
{
    // Error handling is configured by the target integration. This source proves
    // that the integration header remains usable without iostream/exceptions.
    return 0;
}
