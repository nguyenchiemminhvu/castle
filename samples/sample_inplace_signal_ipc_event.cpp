#include "sample_support.hpp"

#include "castle/events/inplace_signal_ipc_event.hpp"

// Scenario: compile smoke test for the in-place signal/IPC event primitive.
int main()
{
    // IPC signal object is target integration glue. This sample remains allocation
    // free and side-effect free on a host while keeping the header buildable.
    return 0;
}
