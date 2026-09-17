#include "sample_support.h"

#include "castle/events/inplace_signal_ipc_event.h"

// Scenario: compile smoke test for the in-place signal/IPC event primitive.
int main()
{
    // IPC signal object is target integration glue. This sample remains allocation
    // free and side-effect free on a host while keeping the header buildable.
    return 0;
}
