#include "sample_support.hpp"

#include "castle/iterator/iterator.hpp"

int main()
{
    uint8_t data[3U] = {4U, 5U, 6U};

    auto second = castle::next(data, 1);
    castle::reverse_iterator<uint8_t*> reverse_end(data + 3U);

    CASTLE_SAMPLE_CHECK(*second == 5U);
    CASTLE_SAMPLE_CHECK(*reverse_end == 6U);

    return 0;
}
