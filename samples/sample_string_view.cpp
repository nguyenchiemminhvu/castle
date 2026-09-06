#include "sample_support.h"

#include "castle/container/string_view.h"

// Scenario: parsing an incoming command frame without copying it into another buffer.
int main()
{
    const castle::container::string_view command("SET:TEMP=25");
    CASTLE_SAMPLE_CHECK(command.starts_with(castle::container::string_view("SET:")));
    CASTLE_SAMPLE_CHECK(command.ends_with(castle::container::string_view("25")));
    CASTLE_SAMPLE_CHECK(command.find(castle::container::string_view("TEMP")) == 4U);
    const auto value = command.substr(9U);
    CASTLE_SAMPLE_CHECK(value.size() == 2U);
    return 0;
}
