#include "sample_support.hpp"

#include "castle/math/geometry/detail.hpp"

static_assert(castle::meta::is_same<castle::math::detail::geometry_calc_type<int>, int64_t>::value, "");
static_assert(castle::meta::is_same<castle::math::detail::geometry_calc_type<long long>, long long>::value, "");

int main()
{
    CASTLE_SAMPLE_CHECK(castle::math::detail::cross2<int>(1, 2, 3, 4) == -2);
    CASTLE_SAMPLE_CHECK(castle::math::detail::dot2<int>(1, 2, 3, 4) == 11);
    CASTLE_SAMPLE_CHECK(castle::math::detail::dot3<int>(1, 2, 3, 4, 5, 6) == 32);
    CASTLE_SAMPLE_CHECK(castle::math::detail::cross3_x<int>(2, 3, 5, 6) == -3);
    CASTLE_SAMPLE_CHECK(castle::math::detail::cross3_y<int>(3, 1, 6, 4) == 6);
    CASTLE_SAMPLE_CHECK(castle::math::detail::cross3_z<int>(1, 2, 4, 5) == -3);
    CASTLE_SAMPLE_CHECK(castle::math::detail::near_zero(0.25f, 0.3f));
    return 0;
}
