#include "sample_support.hpp"

#include "castle/utility/string_builder.hpp"

#include <stdint.h>

enum class link_state : uint8_t
{
    down = 0U,
    up = 1U
};

int main()
{
    castle::string_builder<64U> line;
    line.build("temp=", 25.3F, "C state=", link_state::up, " ok=", true);
    CASTLE_SAMPLE_CHECK(line.size() > 0U);
    CASTLE_SAMPLE_CHECK(line.capacity() == 64U);
    CASTLE_SAMPLE_CHECK(!line.empty());
    CASTLE_SAMPLE_CHECK(!line.full());
    CASTLE_SAMPLE_CHECK(!line.truncated());
    CASTLE_SAMPLE_CHECK(line.build_status() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(line.c_str()[0] == 't');
    CASTLE_SAMPLE_CHECK(line.view() == castle::container::string_view("temp=25.300C state=1 ok=true"));

    castle::string_builder<32U> chained;
    chained << "x=" << 10 << " y=" << -20;
    CASTLE_SAMPLE_CHECK(chained.view() == castle::container::string_view("x=10 y=-20"));

    castle::string_builder<32U> precise;
    precise.append(3.14159F, 2U);
    CASTLE_SAMPLE_CHECK(precise.view() == castle::container::string_view("3.14"));
    precise.clear();
    CASTLE_SAMPLE_CHECK(precise.empty());

    castle::string_builder<12U> short_line;
    short_line.append("this line is definitely too long");
    CASTLE_SAMPLE_CHECK(short_line.truncated());
    CASTLE_SAMPLE_CHECK(short_line.build_status() == castle::status::data_loss);
    CASTLE_SAMPLE_CHECK(short_line.view() == castle::container::string_view("this line..."));

    castle::string_builder<13U> padded;
    padded.append("value");
    for (unsigned i = 0U; i < 8U; ++i)
    {
        padded.append(' ');
    }
    padded.append("!");
    CASTLE_SAMPLE_CHECK(padded.view() == castle::container::string_view("value..."));

    castle::string_builder<13U, false> padded_no_trim;
    padded_no_trim.append("value");
    for (unsigned i = 0U; i < 8U; ++i)
    {
        padded_no_trim.append(' ');
    }
    padded_no_trim.append("!");
    CASTLE_SAMPLE_CHECK(padded_no_trim.view() == castle::container::string_view("value     ..."));

    castle::wstring_builder<16U> wide;
    wide.build(L"ok=", true);
    CASTLE_SAMPLE_CHECK(wide.view() == castle::container::basic_string_view<wchar_t>(L"ok=true"));
    return 0;
}
