#include <gtest/gtest.h>

#include "castle/sync/wait_policy.hpp"

namespace
{

// ---------------------------------------------------------------------------
// castle::spin_wait exposes a callable, noexcept, static wait() and never
// touches any external state.
// ---------------------------------------------------------------------------
TEST(SpinWaitTest, WaitIsCallableAndNoexcept)
{
    static_assert(noexcept(castle::spin_wait::wait()), "wait");

    // Never blocks; must return immediately.
    castle::spin_wait::wait();
}


// ---------------------------------------------------------------------------
// A conforming WaitPolicy only needs a public, callable, noexcept, static
// wait() with no arguments; user-defined policies satisfying that contract
// are freely substitutable wherever castle::spin_wait is used.
// ---------------------------------------------------------------------------
TEST(SpinWaitTest, CustomPolicySatisfyingTheContractIsUsable)
{
    struct custom_policy
    {
        static void wait() noexcept {}
    };

    static_assert(noexcept(custom_policy::wait()), "wait");

    custom_policy::wait();
}

} // namespace
