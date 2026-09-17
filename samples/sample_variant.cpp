#include "sample_support.h"

#include "castle/utility/variant.h"

// Scenario: implementing a finite-state message payload without dynamic allocation.
#include <stdint.h>
int main()
{
    castle::variant<uint32_t, uint16_t> message(castle::in_place_index<0>, 77U);
    CASTLE_SAMPLE_CHECK(message.index() == 0U);
    CASTLE_SAMPLE_CHECK(castle::holds_alternative<uint32_t>(message));
    CASTLE_SAMPLE_CHECK(castle::get<uint32_t>(message) == 77U);
    message.emplace<1U>(9U);
    CASTLE_SAMPLE_CHECK(castle::get<1U>(message) == 9U);
    message.reset();
    CASTLE_SAMPLE_CHECK(message.valueless_by_exception());
    return 0;
}
