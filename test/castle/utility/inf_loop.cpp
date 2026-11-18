#include <gtest/gtest.h>

#include "castle/utility/inf_loop.hpp"

#include <setjmp.h>

namespace
{

struct valid_wait_policy
{
    static void wait() noexcept
    {
    }
};

struct missing_wait_policy
{
};

struct throwing_wait_policy
{
    static void wait()
    {
    }
};

jmp_buf loop_escape;
volatile unsigned int wait_count = 0U;

struct counted_wait_policy
{
    static void wait() noexcept
    {
        ++wait_count;
        if (wait_count == 3U)
        {
            // Test-only escape from an API whose production contract is
            // explicitly non-returning. All state used across longjmp is static.
            longjmp(loop_escape, 1);
        }
    }
};

static_assert(castle::detail::has_static_wait<valid_wait_policy>::value,
              "A noexcept static wait function must satisfy the policy");
static_assert(!castle::detail::has_static_wait<missing_wait_policy>::value,
              "A policy without wait must be rejected");
static_assert(!castle::detail::has_static_wait<throwing_wait_policy>::value,
              "A potentially-throwing wait function must be rejected");
static_assert(noexcept(castle::infinite_loop<valid_wait_policy>()),
              "The infinite loop must be noexcept");

TEST(InfLoopTest, RepeatsWaitPolicyUntilPlatformControlTransfer)
{
    wait_count = 0U;

    if (setjmp(loop_escape) == 0)
    {
        castle::infinite_loop<counted_wait_policy>();
    }

    EXPECT_EQ(wait_count, 3U);
}

TEST(InfLoopTest, ExposesNoexceptFunctionPointer)
{
    using function_pointer = void (*)() noexcept;
    function_pointer const entry = &castle::infinite_loop<valid_wait_policy>;

    EXPECT_NE(entry, nullptr);
    EXPECT_TRUE(noexcept(castle::infinite_loop<valid_wait_policy>()));
}

} // namespace
