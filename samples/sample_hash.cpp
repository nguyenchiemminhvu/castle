/**
 * Castle 2.0 sample: <castle/utility/hash.h>
 *
 * Scenario: produce stable keys for fixed-capacity hash tables used by a
 * command/telemetry registry. No dynamic storage is involved.
 */
#include "sample_support.h"
#include "castle/utility/hash.h"
#include "castle/container/string_view.h"

#include <stdint.h>

int main()
{
    const castle::size_type h1 = castle::hash<uint32_t>()(123U);
    const castle::size_type h2 = castle::hash<uint32_t>()(123U);
    CASTLE_SAMPLE_CHECK(h1 == h2);

    const castle::container::string_view key("TEMP");
    const castle::size_type text_hash = castle::hash<castle::container::string_view>()(key);
    CASTLE_SAMPLE_CHECK(text_hash != 0U);
    return 0;
}
