/**
 * Castle 2.0 sample: traits headers.
 *
 * Filename collision note: core/traits.h and iterator/traits.h share the same
 * basename, so the required sample_<header_file_name>.cpp rule permits one file.
 * Both APIs are covered here.
 */
#include "sample_support.h"
#include "castle/core/traits.h"
#include "castle/iterator/traits.h"

#include <stdint.h>

int main()
{
    static_assert(castle::meta::is_integral<int>::value,
                  "integral trait");
    static_assert(castle::meta::is_unsigned<unsigned int>::value,
                  "unsigned trait");
    static_assert(castle::meta::is_same<castle::remove_cv_t<const uint32_t>, uint32_t>::value,
                  "remove cv");
    static_assert(castle::meta::is_same<castle::iterator_traits<int*>::value_type, int>::value,
                  "iterator value_type");
    return 0;
}
