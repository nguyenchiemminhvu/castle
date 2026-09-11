#include "sample_support.h"

#include "castle/container/stack.h"

// Scenario: bounded execution stack for a cooperative command interpreter.
int main()
{
    castle::container::stack_buffer<uint32_t, 4U> stack;
    CASTLE_SAMPLE_CHECK(stack.push(10U));
    CASTLE_SAMPLE_CHECK(stack.push(20U));
    CASTLE_SAMPLE_CHECK(stack.top() == 20U);
    CASTLE_SAMPLE_CHECK(stack.pop() == true);
    CASTLE_SAMPLE_CHECK(stack.top() == 10U);
    return 0;
}
