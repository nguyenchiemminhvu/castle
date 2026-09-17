#include "sample_support.hpp"

#include "castle/container/array.hpp"

int main()
{
    castle::container::array<uint32_t, 3U> constructed(10U, 20U, 30U);
    CASTLE_SAMPLE_CHECK(constructed.size() == 3U);
    CASTLE_SAMPLE_CHECK(constructed.capacity() == 3U);
    CASTLE_SAMPLE_CHECK(constructed.full());
    CASTLE_SAMPLE_CHECK(!constructed.empty());
    CASTLE_SAMPLE_CHECK(constructed.front() == 10U);
    CASTLE_SAMPLE_CHECK(constructed.back() == 30U);

    castle::container::array<uint32_t, 4U> seeded{100U, 200U, 300U, 400U};
    seeded[2U] = 330U;
    CASTLE_SAMPLE_CHECK(seeded[2U] == 330U);
    CASTLE_SAMPLE_CHECK(seeded.data() == seeded.begin());
    CASTLE_SAMPLE_CHECK(*(seeded.end() - 1) == 400U);
    CASTLE_SAMPLE_CHECK(*seeded.rbegin() == 400U);

    uint32_t sum = 0U;
    for (auto it = seeded.begin(); it != seeded.end(); ++it)
    {
        sum += *it;
    }
    CASTLE_SAMPLE_CHECK(sum == 1030U);
    CASTLE_SAMPLE_CHECK((castle::container::array<uint32_t, 4U>::static_size == 4U));

    castle::container::array<uint32_t, 0U> empty;
    CASTLE_SAMPLE_CHECK(empty.size() == 0U);
    CASTLE_SAMPLE_CHECK(empty.capacity() == 0U);
    CASTLE_SAMPLE_CHECK(empty.empty());
    CASTLE_SAMPLE_CHECK(empty.full());
    CASTLE_SAMPLE_CHECK(empty.begin() == nullptr);
    CASTLE_SAMPLE_CHECK(empty.data() == nullptr);

    return 0;
}
