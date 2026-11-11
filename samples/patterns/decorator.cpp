#include "sample_support.hpp"

#include "castle/patterns/decorator.hpp"
#include "castle/callbacks/function.hpp"

namespace dp = castle::patterns;
namespace cb = castle::callbacks;

int main()
{
    // The function to decorate.
    cb::function<int(int)> square = [](int value) noexcept { return value * value; };

    // Decorator 1: runs before the call (counts calls).
    int calls = 0;
    auto count_calls = [&calls](auto& next, int value) noexcept
    {
        ++calls;
        return next(value);
    };

    // Decorator 2: runs after the call (limits the result).
    auto limit_to_100 = [](auto& next, int value) noexcept
    {
        int const result = next(value);
        return result > 100 ? 100 : result;
    };

    // Same as Python:
    //   @count_calls
    //   @limit_to_100
    //   def square(value): ...
    auto decorated_square = dp::decorate(square, count_calls, limit_to_100);

    CASTLE_SAMPLE_CHECK(decorated_square(3) == 9);
    CASTLE_SAMPLE_CHECK(decorated_square(20) == 100);
    CASTLE_SAMPLE_CHECK(calls == 2);

    // The original function is unchanged.
    CASTLE_SAMPLE_CHECK(square(20) == 400);

    return 0;
}
