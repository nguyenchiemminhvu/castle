#include "sample_support.hpp"

#include "castle/container/string_view.hpp"

int main()
{
    const char command_text[] = "SET:TEMP=25";
    castle::container::string_view command(command_text);
    CASTLE_SAMPLE_CHECK(command.size() == 11U);
    CASTLE_SAMPLE_CHECK(command.length() == 11U);
    CASTLE_SAMPLE_CHECK(!command.empty());
    CASTLE_SAMPLE_CHECK(command.data() == command_text);
    CASTLE_SAMPLE_CHECK(command.front() == 'S');
    CASTLE_SAMPLE_CHECK(command.back() == '5');
    CASTLE_SAMPLE_CHECK(command[4U] == 'T');
    CASTLE_SAMPLE_CHECK(*command.begin() == 'S');
    CASTLE_SAMPLE_CHECK(*(command.end() - 1) == '5');

    castle::container::string_view prefix(command_text, 3U);
    CASTLE_SAMPLE_CHECK(prefix == castle::container::string_view("SET"));
    CASTLE_SAMPLE_CHECK(prefix != castle::container::string_view("GET"));

    auto key = command.substr(4U, 4U);
    CASTLE_SAMPLE_CHECK(key == castle::container::string_view("TEMP"));
    auto value = command.substr(9U);
    CASTLE_SAMPLE_CHECK(value == castle::container::string_view("25"));
    CASTLE_SAMPLE_CHECK(command.substr(4U, 99U).size() == 7U);
    CASTLE_SAMPLE_CHECK(command.substr(99U).empty());

    CASTLE_SAMPLE_CHECK(command.find('T') == 2U);
    CASTLE_SAMPLE_CHECK(command.find('T', 4U) == 4U);
    CASTLE_SAMPLE_CHECK(command.find('Z') == castle::container::string_view::npos);
    CASTLE_SAMPLE_CHECK(command.find(castle::container::string_view("TEMP")) == 4U);
    CASTLE_SAMPLE_CHECK(command.find(castle::container::string_view("TEMP"), 5U) == castle::container::string_view::npos);

    CASTLE_SAMPLE_CHECK(command.compare(castle::container::string_view("SET:TEMP=25")) == 0);
    CASTLE_SAMPLE_CHECK(prefix < castle::container::string_view("TF"));
    CASTLE_SAMPLE_CHECK(command.starts_with(castle::container::string_view("SET:")));
    CASTLE_SAMPLE_CHECK(command.ends_with(castle::container::string_view("25")));
    CASTLE_SAMPLE_CHECK(command >= prefix);

    castle::container::string_view empty;
    CASTLE_SAMPLE_CHECK(empty.empty());
    CASTLE_SAMPLE_CHECK(empty.find('X') == castle::container::string_view::npos);

    return 0;
}
