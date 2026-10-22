#include "sample_support.hpp"

#include "castle_ext/comm/can/filter.hpp"

using namespace castle::comm::can;

// Filters are literal types: acceptance rules can live in constant expressions.
static constexpr filter diagnostic_block = filter(0x7E0U, 0x7F8U); // 0x7E0..0x7E7

int main()
{
    static_assert(invalid_filter < 0, "invalid_filter never names an installed filter");
    static_assert(diagnostic_block.matches(frame(0x7E3U)), "masked filters can be evaluated at compile time");

    // Default filter: mask 0, so every standard frame passes.
    const filter default_filter;
    CASTLE_SAMPLE_CHECK(default_filter.id == 0U && default_filter.mask == 0U);
    CASTLE_SAMPLE_CHECK(default_filter.format == id_format::standard);
    CASTLE_SAMPLE_CHECK(default_filter.matches(frame(0x555U)));
    CASTLE_SAMPLE_CHECK(!default_filter.matches(frame(0x555U, id_format::extended)));

    // exact(): one identifier only.
    const filter one_id = filter::exact(0x123U);
    CASTLE_SAMPLE_CHECK(one_id.id == 0x123U);
    CASTLE_SAMPLE_CHECK(one_id.mask == max_standard_id);
    CASTLE_SAMPLE_CHECK(one_id.matches(frame(0x123U)));
    CASTLE_SAMPLE_CHECK(!one_id.matches(frame(0x124U)));

    const filter one_extended = filter::exact(0x18FF0001U, id_format::extended);
    CASTLE_SAMPLE_CHECK(one_extended.mask == max_extended_id);
    CASTLE_SAMPLE_CHECK(one_extended.matches(frame(0x18FF0001U, id_format::extended)));
    CASTLE_SAMPLE_CHECK(!one_extended.matches(frame(0x18FF0002U, id_format::extended)));

    // Format is part of the match: the same number in the other format never passes.
    CASTLE_SAMPLE_CHECK(!filter::exact(0x123U).matches(frame(0x123U, id_format::extended)));
    CASTLE_SAMPLE_CHECK(!filter::exact(0x123U, id_format::extended).matches(frame(0x123U)));

    // any(): accepts everything of one format.
    const filter any_standard = filter::any();
    const filter any_extended = filter::any(id_format::extended);
    CASTLE_SAMPLE_CHECK(any_standard.mask == 0U && any_extended.mask == 0U);
    CASTLE_SAMPLE_CHECK(any_standard.matches(frame(max_standard_id)));
    CASTLE_SAMPLE_CHECK(any_extended.matches(frame(max_extended_id, id_format::extended)));
    CASTLE_SAMPLE_CHECK(!any_standard.matches(frame(1U, id_format::extended)));

    // Masked filter: zero mask bits are "don't care". 0x120..0x12F share the upper bits.
    const filter block(0x120U, 0x7F0U);
    CASTLE_SAMPLE_CHECK(block.matches(frame(0x120U)));
    CASTLE_SAMPLE_CHECK(block.matches(frame(0x12FU)));
    CASTLE_SAMPLE_CHECK(!block.matches(frame(0x130U)));
    CASTLE_SAMPLE_CHECK(!block.matches(frame(0x11FU)));

    // Only identifier and format are compared: data and remote frames both pass.
    frame remote(0x123U);
    CASTLE_SAMPLE_CHECK(remote.set_remote(2U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(one_id.matches(remote));

    // validate_filter() checks that id and mask fit the format.
    CASTLE_SAMPLE_CHECK(validate_filter(one_id) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(validate_filter(any_extended) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(validate_filter(filter(max_standard_id + 1U, 0U)) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(validate_filter(filter(0U, max_standard_id + 1U)) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(validate_filter(filter(max_extended_id + 1U, 0U, id_format::extended))
                        == castle::status::invalid_argument);
    filter bad_format;
    bad_format.format = static_cast<id_format>(9U);
    CASTLE_SAMPLE_CHECK(validate_filter(bad_format) == castle::status::invalid_argument);

    // A filter handle is a plain int; invalid_filter marks "none".
    filter_handle handle = invalid_filter;
    CASTLE_SAMPLE_CHECK(handle == -1);

    return 0;
}
