#include "sample_support.hpp"

#include "castle/utility/string_builder.hpp"

enum class link_state : uint8_t
{
    down = 0U,
    up = 1U
};

// Scenario: building bounded diagnostic/telemetry lines without heap allocation,
// exceptions, or iostream - values of different types are folded into a single
// fixed-capacity string and long lines are truncated with a trailing "...".
int main()
{
    // Type-safe, heterogeneous values folded into one fixed-capacity line.
    castle::string_builder<64U> line;
    line.build("temp=", 25.3F, "C state=", link_state::up, " ok=", true);
    CASTLE_SAMPLE_CHECK(line.truncated() == false);
    CASTLE_SAMPLE_CHECK(line.view() == castle::container::string_view("temp=25.300C state=1 ok=true"));

    // operator<< reads like an iostream, but stays heap-free/exception-free.
    castle::string_builder<32U> chained;
    chained << "x=" << 10 << " y=" << -20;
    CASTLE_SAMPLE_CHECK(chained.view() == castle::container::string_view("x=10 y=-20"));

    // Overflowing content is truncated and sealed with "...".
    castle::string_builder<12U> short_line;
    short_line.append("this line is definitely too long");
    CASTLE_SAMPLE_CHECK(short_line.truncated() == true);
    CASTLE_SAMPLE_CHECK(short_line.build_status() == castle::status::data_loss);
    CASTLE_SAMPLE_CHECK(short_line.view() == castle::container::string_view("this line..."));

    // Trailing whitespace is trimmed before "..." is placed (default policy),
    // so padded fields do not end up with dangling spaces before the marker.
    castle::string_builder<13U> padded;
    padded.append("value");
    for (unsigned i = 0U; i < 8U; ++i)
    {
        padded.append(' ');
    }
    padded.append("!");
    CASTLE_SAMPLE_CHECK(padded.view() == castle::container::string_view("value..."));

    // Disabling the trim policy keeps whitespace glued right before "...".
    castle::string_builder<13U, false> padded_no_trim;
    padded_no_trim.append("value");
    for (unsigned i = 0U; i < 8U; ++i)
    {
        padded_no_trim.append(' ');
    }
    padded_no_trim.append("!");
    CASTLE_SAMPLE_CHECK(padded_no_trim.view() == castle::container::string_view("value     ..."));

    return 0;
}
