#include "sample_support.hpp"

#include "castle/utility/compare.hpp"

struct descending_less
{
    bool operator()(const int& lhs, const int& rhs) const noexcept
    {
        return lhs > rhs;
    }
};

int main()
{
    CASTLE_SAMPLE_CHECK(castle::less<int>()(1, 2));
    CASTLE_SAMPLE_CHECK(castle::greater<int>()(2, 1));
    CASTLE_SAMPLE_CHECK(castle::equal_to<int>()(4, 4));

    CASTLE_SAMPLE_CHECK(castle::compare<int>::lt(1, 2));
    CASTLE_SAMPLE_CHECK(castle::compare<int>::gt(2, 1));
    CASTLE_SAMPLE_CHECK(castle::compare<int>::lte(2, 2));
    CASTLE_SAMPLE_CHECK(castle::compare<int>::gte(2, 2));
    CASTLE_SAMPLE_CHECK(castle::compare<int>::eq(5, 5));
    CASTLE_SAMPLE_CHECK(castle::compare<int>::ne(5, 6));
    CASTLE_SAMPLE_CHECK(castle::compare<int>::cmp(1, 2) == castle::compare<int>::Less);
    CASTLE_SAMPLE_CHECK(castle::compare<int>::cmp(2, 2) == castle::compare<int>::Equal);
    CASTLE_SAMPLE_CHECK(castle::compare<int>::cmp(3, 2) == castle::compare<int>::Greater);
    CASTLE_SAMPLE_CHECK(castle::cmp_3_ways<int>(9, 4) == 1);

    CASTLE_SAMPLE_CHECK((castle::compare<int, descending_less>::lt(5, 3)));
    CASTLE_SAMPLE_CHECK((castle::cmp_3_ways<int, descending_less>(3, 5) == 1));
    return 0;
}
