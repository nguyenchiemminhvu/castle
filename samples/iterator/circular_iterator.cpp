#include "sample_support.hpp"

#include "castle/iterator/circular_iterator.hpp"

int main()
{
    uint8_t data[3U] = {10U, 20U, 30U};
    castle::circular_iterator<uint8_t*> it(data, data + 3U, data + 2U);

    CASTLE_SAMPLE_CHECK(*it == 30U);
    ++it;
    CASTLE_SAMPLE_CHECK(*it == 10U);
    ++it;
    CASTLE_SAMPLE_CHECK(*it == 20U);
    --it;
    CASTLE_SAMPLE_CHECK(*it == 10U);
    --it;
    CASTLE_SAMPLE_CHECK(*it == 30U);
    CASTLE_SAMPLE_CHECK(it.base() == data + 2U);

    return 0;
}
