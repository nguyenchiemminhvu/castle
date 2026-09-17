#include "sample_support.hpp"

#include "castle/iterator/reverse_iterator.hpp"

int main()
{
    uint8_t data[4U] = {1U, 2U, 3U, 4U};
    castle::reverse_iterator<uint8_t*> it(data + 4U);

    CASTLE_SAMPLE_CHECK(*it == 4U);
    CASTLE_SAMPLE_CHECK(it[1] == 3U);

    ++it;
    CASTLE_SAMPLE_CHECK(*it == 3U);

    auto next = it + 1;
    CASTLE_SAMPLE_CHECK(*next == 2U);
    CASTLE_SAMPLE_CHECK(next.base() == data + 2U);
    CASTLE_SAMPLE_CHECK(next > it);

    return 0;
}
