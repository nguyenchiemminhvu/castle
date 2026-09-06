#include "sample_support.h"

#include "castle/container/avl_tree.h"

// Scenario: bounded key/value lookup for calibration parameters with deterministic node storage.
#include <stdint.h>

int main()
{
    castle::container::avl_tree<uint16_t, uint32_t, 8U> table;
    CASTLE_SAMPLE_CHECK(table.insert(20U, 2000U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(table.insert(10U, 1000U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(table.insert(30U, 3000U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(table.contains(10U));
    auto it = table.find(30U);
    CASTLE_SAMPLE_CHECK(it != table.end());
    CASTLE_SAMPLE_CHECK(it->second == 3000U);
    auto lb = table.lower_bound(15U);
    CASTLE_SAMPLE_CHECK(lb != table.end() && lb->first == 20U);
    return 0;
}
