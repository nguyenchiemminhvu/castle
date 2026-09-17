#include "sample_support.h"

#include "castle/design_patterns/visitor.h"

// Scenario: compile-time smoke test for a visitor-based command/event hierarchy.
int main()
{
    // Visitor support is a type-level abstraction; concrete visitable nodes in the
    // application supply the target-specific accept()/visit() methods.
    return 0;
}
