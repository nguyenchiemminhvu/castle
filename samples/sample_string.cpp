#include "sample_support.h"

#include "castle/container/string.h"

// Scenario: building a bounded diagnostic/telemetry line with explicit capacity errors.
int main()
{
    castle::container::string frame("TEMP=");
    CASTLE_SAMPLE_CHECK(frame.append("25C") == castle::status::ok);
    CASTLE_SAMPLE_CHECK(frame.size() == 8U);
    CASTLE_SAMPLE_CHECK(frame.find('2') == 5U);
    CASTLE_SAMPLE_CHECK(frame.compare(castle::container::string_view("TEMP=25C")) == 0);
    return 0;
}
