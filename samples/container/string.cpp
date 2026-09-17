#include "sample_support.hpp"

#include "castle/container/string.hpp"

int main()
{
    using string_t = castle::container::string<16U>;

    string_t text;
    CASTLE_SAMPLE_CHECK(text.empty());
    CASTLE_SAMPLE_CHECK(text.capacity() == 16U);
    CASTLE_SAMPLE_CHECK(text.static_capacity == 16U);

    string_t from_cstr("AB");
    castle::container::string_view xy("XY");
    string_t from_view(xy);
    string_t copied(from_cstr);
    copied = from_view;
    CASTLE_SAMPLE_CHECK(copied == xy);

    CASTLE_SAMPLE_CHECK(from_cstr.push_back('C') == castle::status::ok);
    CASTLE_SAMPLE_CHECK(from_cstr.append("DE") == castle::status::ok);
    CASTLE_SAMPLE_CHECK(from_cstr.append(castle::container::string_view("FG")) == castle::status::ok);

    string_t suffix("HI");
    CASTLE_SAMPLE_CHECK(from_cstr.append(suffix) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(from_cstr.size() == 9U);
    CASTLE_SAMPLE_CHECK(from_cstr.length() == 9U);
    CASTLE_SAMPLE_CHECK(from_cstr.available() == 7U);
    CASTLE_SAMPLE_CHECK(from_cstr.front() == 'A');
    CASTLE_SAMPLE_CHECK(from_cstr.back() == 'I');
    CASTLE_SAMPLE_CHECK(from_cstr[1U] == 'B');
    CASTLE_SAMPLE_CHECK(from_cstr.data()[2U] == 'C');
    CASTLE_SAMPLE_CHECK(from_cstr.c_str()[from_cstr.size()] == '\0');

    CASTLE_SAMPLE_CHECK(from_cstr.insert(2U, castle::container::string_view("XY")) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(from_cstr.find('X') == 2U);
    CASTLE_SAMPLE_CHECK(from_cstr.find(castle::container::string_view("DE")) == 5U);
    CASTLE_SAMPLE_CHECK(from_cstr.compare(castle::container::string_view("ABXYCDEFGHI")) == 0);

    auto view = from_cstr.view();
    CASTLE_SAMPLE_CHECK(view.size() == from_cstr.size());
    CASTLE_SAMPLE_CHECK(from_cstr == view);
    CASTLE_SAMPLE_CHECK(view == from_cstr);
    CASTLE_SAMPLE_CHECK(from_cstr != castle::container::string_view("nope"));

    CASTLE_SAMPLE_CHECK(from_cstr.erase(2U, 2U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(from_cstr.compare(castle::container::string_view("ABCDEFGHI")) == 0);

    CASTLE_SAMPLE_CHECK(from_cstr.resize(5U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(from_cstr.compare(castle::container::string_view("ABCDE")) == 0);
    CASTLE_SAMPLE_CHECK(from_cstr.resize(7U, 'Z') == castle::status::ok);
    CASTLE_SAMPLE_CHECK(from_cstr.compare(castle::container::string_view("ABCDEZZ")) == 0);
    CASTLE_SAMPLE_CHECK(from_cstr.pop_back() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(from_cstr.back() == 'Z');

    const string_t& ctext = from_cstr;
    CASTLE_SAMPLE_CHECK(ctext.cbegin()[0U] == 'A');
    CASTLE_SAMPLE_CHECK(ctext.cend() - ctext.cbegin() == static_cast<castle::difference_type>(from_cstr.size()));

    string_t assign_target;
    CASTLE_SAMPLE_CHECK(assign_target.assign(castle::container::string_view("OK")) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(assign_target.assign("DONE") == castle::status::ok);
    CASTLE_SAMPLE_CHECK(assign_target == castle::container::string_view("DONE"));

    string_t invalid_target("FAIL");
    CASTLE_SAMPLE_CHECK(invalid_target.assign(nullptr) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(invalid_target.empty());

    castle::container::u8string<4U> small("ABCD");
    castle::container::u8string<4U> same_small("ABCD");
    castle::container::u8string<4U> later_small("BCDE");
    CASTLE_SAMPLE_CHECK(small.full());
    CASTLE_SAMPLE_CHECK(small.push_back('E') == castle::status::full);
    CASTLE_SAMPLE_CHECK(small == same_small);
    CASTLE_SAMPLE_CHECK(small <= same_small);
    CASTLE_SAMPLE_CHECK(small < later_small);
    CASTLE_SAMPLE_CHECK(later_small > small);
    CASTLE_SAMPLE_CHECK(small >= same_small);

    assign_target.clear();
    CASTLE_SAMPLE_CHECK(assign_target.empty());
    CASTLE_SAMPLE_CHECK(assign_target.pop_back() == castle::status::empty);
    return 0;
}
