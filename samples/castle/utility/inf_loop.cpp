#include "castle/utility/inf_loop.hpp"

#include <assert.h>
#include <setjmp.h>

namespace
{

jmp_buf stop_loop;
volatile unsigned int wait_calls = 0U;

// Host-side stand-in for a scheduler wait. The non-local transfer is only here
// to let this sample verify a normally non-returning API and then exit main().
// Replace this with an RTOS yield/block primitive in an embedded application.
struct sample_wait_policy
{
    static void wait() noexcept
    {
        ++wait_calls;
        if (wait_calls == 3U)
        {
            longjmp(stop_loop, 1);
        }
    }
};

} // namespace

int main()
{
    static_assert(noexcept(castle::infinite_loop<sample_wait_policy>()),
                  "The infinite loop must be noexcept");

    if (setjmp(stop_loop) == 0)
    {
        castle::infinite_loop<sample_wait_policy>();
    }

    assert(wait_calls == 3U);
    return 0;
}
