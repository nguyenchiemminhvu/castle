#include "sample_support.hpp"

#include "castle/utility/move.hpp"

// Scenario: moving a small message object without pulling in <utility>.
struct Packet
{
    int value;
};

int main()
{
    Packet a{7};
    Packet b{castle::move(a)};
    CASTLE_SAMPLE_CHECK(b.value == 7);
    return 0;
}
