#include "sample_support.hpp"

#include "castle/utility/utility.hpp"

// Scenario: umbrella-header smoke test covering Castle’s standard-library-like utility layer.
int main()
{
    castle::optional<uint8_t> value(3U);
    CASTLE_SAMPLE_CHECK(value.value() == 3U);
    return 0;
}
