#include "sample_support.hpp"

#include "castle/core/types.hpp"

#include <stddef.h>

int main()
{
    castle::size_type count = 4U;
    castle::difference_type delta = -1;
    CASTLE_SAMPLE_CHECK(count == 4U);
    CASTLE_SAMPLE_CHECK(delta < 0);
    CASTLE_SAMPLE_CHECK(sizeof(castle::size_type) == sizeof(size_t));
    CASTLE_SAMPLE_CHECK(sizeof(castle::difference_type) == sizeof(ptrdiff_t));
    return 0;
}
