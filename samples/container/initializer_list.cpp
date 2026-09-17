#include "sample_support.hpp"

#include "castle/container/array_view.hpp"
#include "castle/container/initializer_list.hpp"

namespace
{
uint32_t sum(castle::container::initializer_list<uint32_t> values)
{
    uint32_t total = 0U;
    for (auto it = castle::container::begin(values); it != castle::container::end(values); ++it)
    {
        total += *it;
    }
    return total;
}

uint32_t sum_view(castle::container::array_view<const uint32_t> values)
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
    castle::container::initializer_list<uint32_t> values = {10U, 20U, 30U, 40U};
    CASTLE_SAMPLE_CHECK(castle::container::size(values) == 4U);
    CASTLE_SAMPLE_CHECK(!castle::container::empty(values));
    CASTLE_SAMPLE_CHECK(*castle::container::begin(values) == 10U);
    CASTLE_SAMPLE_CHECK(*(castle::container::end(values) - 1) == 40U);
    CASTLE_SAMPLE_CHECK(castle::container::front(values) == 10U);
    CASTLE_SAMPLE_CHECK(castle::container::back(values) == 40U);
    CASTLE_SAMPLE_CHECK(*castle::container::rbegin(values) == 40U);

    CASTLE_SAMPLE_CHECK(sum({1U, 2U, 3U, 4U}) == 10U);
    CASTLE_SAMPLE_CHECK(sum_view(
        castle::container::make_array_view(
            castle::container::initializer_list<uint32_t>{5U, 6U, 7U})) == 18U);

    return 0;
}
