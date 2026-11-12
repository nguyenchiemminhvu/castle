#include "sample_support.hpp"

#include "castle/dsa/array_view.hpp"
#include "castle/dsa/initializer_list.hpp"

namespace
{
uint32_t sum(castle::dsa::initializer_list<uint32_t> values)
{
    uint32_t total = 0U;
    for (auto it = castle::dsa::begin(values); it != castle::dsa::end(values); ++it)
    {
        total += *it;
    }
    return total;
}

uint32_t sum_view(castle::dsa::array_view<const uint32_t> values)
{
    uint32_t total = 0U;
    for (auto it = values.begin(); it != values.end(); ++it)
    {
        total += *it;
    }
    return total;
}
}

int main()
{
    castle::dsa::initializer_list<uint32_t> values = {10U, 20U, 30U, 40U};
    CASTLE_SAMPLE_CHECK(castle::dsa::size(values) == 4U);
    CASTLE_SAMPLE_CHECK(!castle::dsa::empty(values));
    CASTLE_SAMPLE_CHECK(*castle::dsa::begin(values) == 10U);
    CASTLE_SAMPLE_CHECK(*(castle::dsa::end(values) - 1) == 40U);
    CASTLE_SAMPLE_CHECK(castle::dsa::front(values) == 10U);
    CASTLE_SAMPLE_CHECK(castle::dsa::back(values) == 40U);
    CASTLE_SAMPLE_CHECK(*castle::dsa::rbegin(values) == 40U);

    CASTLE_SAMPLE_CHECK(sum({1U, 2U, 3U, 4U}) == 10U);
    CASTLE_SAMPLE_CHECK(sum_view(
        castle::dsa::make_array_view(
            castle::dsa::initializer_list<uint32_t>{5U, 6U, 7U})) == 18U);

    return 0;
}
