#include "sample_support.hpp"

#include "castle/iterator/tags.hpp"

namespace
{

bool accepts_input(castle::input_iterator_tag)
{
    return true;
}

int category_rank(castle::input_iterator_tag)
{
    return 0;
}

int category_rank(castle::forward_iterator_tag)
{
    return 1;
}

int category_rank(castle::bidirectional_iterator_tag)
{
    return 2;
}

int category_rank(castle::random_access_iterator_tag)
{
    return 3;
}

}

int main()
{
    CASTLE_SAMPLE_CHECK(accepts_input(castle::random_access_iterator_tag{}));
    CASTLE_SAMPLE_CHECK(category_rank(castle::input_iterator_tag{}) == 0);
    CASTLE_SAMPLE_CHECK(category_rank(castle::forward_iterator_tag{}) == 1);
    CASTLE_SAMPLE_CHECK(category_rank(castle::bidirectional_iterator_tag{}) == 2);
    CASTLE_SAMPLE_CHECK(category_rank(castle::random_access_iterator_tag{}) == 3);

    return 0;
}
