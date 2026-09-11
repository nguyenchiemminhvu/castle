#include "sample_support.h"

#include "castle/container/hash_table.h"

// Scenario: deterministic open-addressed key/value storage for a small command table.
int main()
{
    castle::container::hash_table<uint16_t, uint32_t, 8U> table;
    CASTLE_SAMPLE_CHECK(table.insert(7U, 700U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(table.insert_or_assign(7U, 701U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(table.contains(7U));
    CASTLE_SAMPLE_CHECK(*table.get(7U) == 701U);
    CASTLE_SAMPLE_CHECK(table.erase(7U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(!table.contains(7U));
    return 0;
}
