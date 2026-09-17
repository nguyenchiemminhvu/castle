/**
 * Castle 2.0 sample: <castle/container/initializer_list.h>
 *
 * Scenario: seeding fixed-capacity containers from a calibration table
 * expressed as a brace-init-list, plus taking a non-owning view over one
 * without copying it into a container at all.
 */
#include "sample_support.hpp"

#include "castle/container/array_view.hpp"
#include "castle/container/initializer_list.hpp"
#include "castle/container/vector.hpp"
#include "castle/iterator/operations.hpp"

#include <stdint.h>

namespace
{
// Free function taking castle::container::initializer_list<T> directly -
// any {a, b, c} brace-init-list binds to it, same as std::initializer_list.
uint32_t sum(castle::container::initializer_list<uint32_t> samples)
{
    uint32_t total = 0U;
    for (uint32_t value : samples)
    {
        total += value;
    }
    return total;
}
} // namespace

int main()
{
    CASTLE_SAMPLE_CHECK(sum({1U, 2U, 3U, 4U}) == 10U);

    // Fixed-capacity vector constructed and re-seeded from brace-init-lists.
    castle::container::vector<uint32_t, 4U> gains = {100U, 200U, 300U};
    CASTLE_SAMPLE_CHECK(gains.size() == 3U);
    CASTLE_SAMPLE_CHECK(gains[1] == 200U);

    gains = {7U, 8U};
    CASTLE_SAMPLE_CHECK(gains.size() == 2U);
    CASTLE_SAMPLE_CHECK(gains.back() == 8U);

    // A brace-init-list adapts to castle iterators without being copied.
    castle::container::initializer_list<uint32_t> offsets = {10U, 20U, 30U};
    CASTLE_SAMPLE_CHECK(castle::distance(offsets.begin(), offsets.end()) == 3U);
    CASTLE_SAMPLE_CHECK(*castle::container::rbegin(offsets) == 30U);

    // Bridge straight to a non-owning view, e.g. to pass to a routine that
    // only knows about castle::container::array_view. T is spelled explicitly:
    // deducing it through the initializer_list alias directly from a brace
    // literal is not portable across compilers.
    const auto view = castle::container::make_array_view(
        castle::container::initializer_list<uint32_t>{1U, 2U, 3U, 4U, 5U});
    CASTLE_SAMPLE_CHECK(view.size() == 5U);
    CASTLE_SAMPLE_CHECK(view.front() == 1U);
    CASTLE_SAMPLE_CHECK(view.back() == 5U);

    return 0;
}
