#include "sample_support.hpp"

#include "castle/utility/move.hpp"

#include <stdint.h>

struct Packet
{
    uint32_t value;
};

int main()
{
    Packet a{7U};
    Packet b{castle::move(a)};
    static_assert(castle::meta::is_same<decltype(castle::move(a)), Packet&&>::value, "move result type");
    CASTLE_SAMPLE_CHECK(b.value == 7U);
    return 0;
}
