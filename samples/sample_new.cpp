#include "sample_support.h"

#include "castle/memory/new.h"

// Scenario: compile smoke test for placement construction support.
int main()
{
    // This header supplies placement-new support used by Castle’s explicit
    // lifetime machinery. Dynamic operator new is intentionally not used here.
    return 0;
}
