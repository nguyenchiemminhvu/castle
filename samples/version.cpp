#include "sample_support.hpp"

#include "castle/version.hpp"

int main()
{
    static_assert(CASTLE_VERSION_MAJOR == 2, "major version");
    static_assert(CASTLE_VERSION_MINOR == 0, "minor version");
    static_assert(CASTLE_VERSION_PATCH == 0, "patch version");
    static_assert(CASTLE_VERSION_AT_LEAST(2, 0, 0), "minimum version");
    static_assert(!CASTLE_VERSION_AT_LEAST(3, 0, 0), "future version not satisfied");

    CASTLE_SAMPLE_CHECK(castle::version_encoded == CASTLE_VERSION);
    CASTLE_SAMPLE_CHECK(castle::version_major == CASTLE_VERSION_MAJOR);
    CASTLE_SAMPLE_CHECK(castle::version_minor == CASTLE_VERSION_MINOR);
    CASTLE_SAMPLE_CHECK(castle::version_patch == CASTLE_VERSION_PATCH);
    CASTLE_SAMPLE_CHECK(CASTLE_VERSION_STRING[0] == '2');
    return 0;
}
