#include "sample_support.hpp"

#include "castle/iterator/traits.hpp"

namespace
{

bool is_random_access(castle::random_access_iterator_tag)
{
    return true;
}

}

int main()
{
    uint8_t data[2U] = {7U, 9U};

    using mutable_traits = castle::iterator_traits<uint8_t*>;
    mutable_traits::pointer mutable_pointer = data;
    mutable_traits::reference mutable_reference = *mutable_pointer;

    CASTLE_SAMPLE_CHECK(is_random_access(mutable_traits::iterator_category{}));
    CASTLE_SAMPLE_CHECK(mutable_reference == 7U);

    using const_traits = castle::iterator_traits<const uint8_t*>;
    const uint8_t* const_pointer = data;
    const_traits::reference const_reference = *const_pointer;

    CASTLE_SAMPLE_CHECK(is_random_access(const_traits::iterator_category{}));
    CASTLE_SAMPLE_CHECK(const_reference == 7U);

    return 0;
}
