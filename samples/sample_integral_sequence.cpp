#include "sample_support.h"

#include "castle/utility/integral_sequence.h"

// Scenario: compile-time indexing for generic register-field tables.
using seq = castle::sequence::make_index_sequence<4U>::type;
static_assert(seq::size() == 4U, "sequence size");

int main()
{
    return 0;
}
