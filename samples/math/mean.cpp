#include "sample_support.hpp"

#include "castle/math/mean.hpp"

int main()
{
    int values[] = {10, 20, 30, 40};

    castle::math::mean<int, long long> average(values, values + 4);
    CASTLE_SAMPLE_CHECK(average.count() == 4U);
    CASTLE_SAMPLE_CHECK(average.get_mean() == 25.0);

    average(50);
    CASTLE_SAMPLE_CHECK(static_cast<double>(average) == 30.0);

    average.clear();
    CASTLE_SAMPLE_CHECK(average.count() == 0U);
    CASTLE_SAMPLE_CHECK(average.get_mean() == 0.0);
    return 0;
}
