#include "sample_support.hpp"

#include "castle/dsa/array.hpp"
#include "castle/dsa/array_view.hpp"

namespace
{
uint32_t sum_view(castle::dsa::array_view<const uint16_t> view)
{
    uint32_t total = 0U;
    for (auto it = view.begin(); it != view.end(); ++it)
    {
        total += *it;
    }
    return total;
}
}

int main()
{
    uint16_t samples[6U] = {1U, 2U, 3U, 4U, 5U, 6U};
    castle::dsa::array_view<uint16_t> view(samples, 6U);
    CASTLE_SAMPLE_CHECK(view.size() == 6U);
    CASTLE_SAMPLE_CHECK(!view.empty());
    CASTLE_SAMPLE_CHECK(view.front() == 1U);
    CASTLE_SAMPLE_CHECK(view.back() == 6U);

    view[2U] = 30U;
    CASTLE_SAMPLE_CHECK(samples[2U] == 30U);
    CASTLE_SAMPLE_CHECK(view.data() == samples);
    CASTLE_SAMPLE_CHECK(*(view.end() - 1) == 6U);

    auto middle = view.subview(2U, 3U);
    CASTLE_SAMPLE_CHECK(middle.size() == 3U);
    CASTLE_SAMPLE_CHECK(middle.front() == 30U);
    CASTLE_SAMPLE_CHECK(middle.back() == 5U);

    auto tail = view.subview(4U, 99U);
    CASTLE_SAMPLE_CHECK(tail.size() == 2U);
    CASTLE_SAMPLE_CHECK(view.subview(7U, 1U).empty());

    auto raw_view = castle::dsa::make_array_view(samples, 6U);
    CASTLE_SAMPLE_CHECK(raw_view.size() == 6U);

    castle::dsa::array<uint16_t, 4U> fixed{10U, 20U, 30U, 40U};
    castle::dsa::array_view<const uint16_t> const_view(fixed);
    CASTLE_SAMPLE_CHECK(const_view.size() == 4U);
    CASTLE_SAMPLE_CHECK(const_view[1U] == 20U);

    CASTLE_SAMPLE_CHECK(sum_view(
        castle::dsa::make_array_view(
            castle::dsa::initializer_list<uint16_t>{7U, 8U, 9U})) == 24U);

    return 0;
}
