#include "sample_support.hpp"

#include "castle/callbacks/function.hpp"
#include "castle/utility/move.hpp"

#include <stdint.h>

namespace
{
void increment(int& value) noexcept
{
    ++value;
}

int add_pair(int lhs, int rhs) noexcept
{
    return lhs + rhs;
}
} // namespace

int main()
{
    castle::callbacks::function<void(int), 32U> empty{};
    CASTLE_SAMPLE_CHECK(!empty);

    uint32_t accumulator = 0U;
    castle::callbacks::function<void(uint32_t), 32U> owning{
        [&accumulator](uint32_t value) noexcept
        {
            accumulator += value;
        }
    };

    CASTLE_SAMPLE_CHECK(owning);
    owning(7U);
    CASTLE_SAMPLE_CHECK(accumulator == 7U);

    castle::callbacks::function<void(uint32_t), 32U> copied{owning};
    copied(5U);
    CASTLE_SAMPLE_CHECK(accumulator == 12U);

    castle::callbacks::function<void(uint32_t), 32U> moved{castle::move(copied)};
    CASTLE_SAMPLE_CHECK(!copied);
    moved(1U);
    CASTLE_SAMPLE_CHECK(accumulator == 13U);

    castle::callbacks::function<void(int&)> function_pointer{&increment};
    int counter = 0;
    function_pointer(counter);
    CASTLE_SAMPLE_CHECK(counter == 1);

    castle::callbacks::function<void(int&), 32U, 8U> assigned_pointer{};
    assigned_pointer = &increment;
    assigned_pointer(counter);
    CASTLE_SAMPLE_CHECK(counter == 2);

    castle::callbacks::function<int(int, int)> sum{&add_pair};
    CASTLE_SAMPLE_CHECK(sum(3, 4) == 7);

    castle::callbacks::function<int(int, int)> copy_assigned{};
    copy_assigned = sum;
    CASTLE_SAMPLE_CHECK(copy_assigned(5, 6) == 11);

    castle::callbacks::function<int(int, int)> move_assigned{};
    move_assigned = castle::move(copy_assigned);
    CASTLE_SAMPLE_CHECK(!copy_assigned);
    CASTLE_SAMPLE_CHECK(move_assigned(8, 1) == 9);

    assigned_pointer = nullptr;
    CASTLE_SAMPLE_CHECK(!assigned_pointer);

    return 0;
}
