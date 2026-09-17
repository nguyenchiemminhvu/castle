#include "sample_support.hpp"

#include "castle/iterator/fixed_iterator.hpp"

int main()
{
    uint8_t data[4U] = {1U, 2U, 3U, 4U};
    castle::fixed_iterator<uint8_t*> it(data, data, data + 4U);

    ++it;
    ++it;
    CASTLE_SAMPLE_CHECK(*it == 3U);

    while (it.valid())
    {
        ++it;
    }

    CASTLE_SAMPLE_CHECK(!it.valid());
    --it;
    CASTLE_SAMPLE_CHECK(it.valid());
    CASTLE_SAMPLE_CHECK(*it == 4U);
    --it;
    CASTLE_SAMPLE_CHECK(*it == 3U);

    return 0;
}
