#include <gtest/gtest.h>

#include "castle/version.h"

namespace
{

TEST(VersionTest, MacroComponentsMatchExpectedRelease)
{
    EXPECT_EQ(CASTLE_VERSION_MAJOR, 2);
    EXPECT_EQ(CASTLE_VERSION_MINOR, 0);
    EXPECT_EQ(CASTLE_VERSION_PATCH, 0);
}

TEST(VersionTest, EncodePacksFieldsIntoSingleInteger)
{
    EXPECT_EQ(CASTLE_VERSION_ENCODE(1, 2, 3), (1 << 16) | (2 << 8) | 3);
    EXPECT_EQ(CASTLE_VERSION_ENCODE(0, 0, 0), 0);
    EXPECT_EQ(CASTLE_VERSION, CASTLE_VERSION_ENCODE(2, 0, 0));
}

TEST(VersionTest, VersionStringMatchesComponents)
{
    EXPECT_STREQ(CASTLE_VERSION_STRING, "2.0.0");
}

TEST(VersionTest, StringifyExpandsMacroValues)
{
    EXPECT_STREQ(CASTLE_STRINGIFY(2), "2");
    EXPECT_STREQ(CASTLE_STRINGIFY(CASTLE_VERSION_MAJOR), "2");
}

TEST(VersionTest, VersionAtLeastComparesCorrectly)
{
    EXPECT_TRUE(CASTLE_VERSION_AT_LEAST(2, 0, 0));
    EXPECT_TRUE(CASTLE_VERSION_AT_LEAST(1, 9, 9));
    EXPECT_TRUE(CASTLE_VERSION_AT_LEAST(0, 0, 0));
    EXPECT_FALSE(CASTLE_VERSION_AT_LEAST(2, 0, 1));
    EXPECT_FALSE(CASTLE_VERSION_AT_LEAST(3, 0, 0));
}

TEST(VersionTest, ConstexprConstantsMatchMacros)
{
    static_assert(castle::version_major == CASTLE_VERSION_MAJOR, "major mismatch");
    static_assert(castle::version_minor == CASTLE_VERSION_MINOR, "minor mismatch");
    static_assert(castle::version_patch == CASTLE_VERSION_PATCH, "patch mismatch");
    static_assert(castle::version_encoded == CASTLE_VERSION, "encoded mismatch");

    EXPECT_EQ(castle::version_major, 2);
    EXPECT_EQ(castle::version_minor, 0);
    EXPECT_EQ(castle::version_patch, 0);
    EXPECT_EQ(castle::version_encoded, CASTLE_VERSION);
}

} // namespace
