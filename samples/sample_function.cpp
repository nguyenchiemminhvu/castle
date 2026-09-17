#include "sample_support.hpp"

#include "castle/utility/move.hpp"
#include "castle/callbacks/function.hpp"

// Scenario: binding a small ISR-safe callable entirely inside inline storage.
#include <stdint.h>

void foo(int& val)
{
    val++;
}

int main()
{
    uint32_t accumulator = 0U;
    castle::callbacks::function<void(uint32_t), 32U> callback =
        [&accumulator](uint32_t value) { accumulator += value; };

    callback(7U);
    callback(5U);
    CASTLE_SAMPLE_CHECK(accumulator == 12U);

    int val=0;
    castle::callbacks::function<void(int&)> f(foo);
    castle::callbacks::function<void(int&),32,8> ff=foo;
    f(val);
    ff(val);
    CASTLE_SAMPLE_CHECK(val == 2);

    return 0;
}
