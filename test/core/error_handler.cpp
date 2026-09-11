#include <gtest/gtest.h>

#include <type_traits>

#include "castle/core/error_handler.h"

namespace
{

// A user-defined error type constructible from (file, line), which is what the
// CASTLE_ERROR(TYPE) macro expands to in the default configuration.
struct MyError
{
    const char* file;
    int line;

    CASTLE_CONSTEXPR MyError(const char* f, int l) CASTLE_NOEXCEPT : file(f), line(l) {}
};

TEST(CoreErrorHandlerTest, ExceptionStoresAllFields)
{
    castle::exception e("bad thing", "widget.cpp", 128);
    EXPECT_STREQ(e.what(), "bad thing");
    EXPECT_STREQ(e.file_name(), "widget.cpp");
    EXPECT_EQ(e.line(), 128);
}

TEST(CoreErrorHandlerTest, ExceptionIsUsableInConstexprContext)
{
    constexpr castle::exception e("boom", "core.cpp", 42);
    static_assert(e.line() == 42, "line() must be constexpr");
    EXPECT_STREQ(e.what(), "boom");
    EXPECT_STREQ(e.file_name(), "core.cpp");
}

TEST(CoreErrorHandlerTest, NullReasonAndFileBecomeEmptyStrings)
{
    castle::exception e(nullptr, nullptr, 7);
    EXPECT_STREQ(e.what(), "");
    EXPECT_STREQ(e.file_name(), "");
    EXPECT_EQ(e.line(), 7);
}

TEST(CoreErrorHandlerTest, NullReasonOnlyLeavesFileIntact)
{
    castle::exception e(nullptr, "only_file.cpp", -3);
    EXPECT_STREQ(e.what(), "");
    EXPECT_STREQ(e.file_name(), "only_file.cpp");
    EXPECT_EQ(e.line(), -3);
}

TEST(CoreErrorHandlerTest, ExceptionIsTriviallyCopyable)
{
    // Documented invariant: safe to pass by value in interrupt contexts.
    static_assert(std::is_trivially_copyable<castle::exception>::value,
                  "exception must be trivially copyable");
    castle::exception original("copy", "copy.cpp", 1);
    castle::exception copy = original;
    EXPECT_STREQ(copy.what(), "copy");
    EXPECT_STREQ(copy.file_name(), "copy.cpp");
    EXPECT_EQ(copy.line(), 1);
}

TEST(CoreErrorHandlerTest, ErrorGenericMacroCapturesReason)
{
    // Default configuration: reason is captured, file is "" and line is -1.
    castle::exception e = CASTLE_ERROR_GENERIC("generic failure");
    EXPECT_STREQ(e.what(), "generic failure");
    EXPECT_STREQ(e.file_name(), "");
    EXPECT_EQ(e.line(), -1);
}

TEST(CoreErrorHandlerTest, ErrorMacroConstructsUserTypeWithFileAndLine)
{
    // Default configuration: CASTLE_ERROR(TYPE) expands to TYPE("", -1).
    MyError err = CASTLE_ERROR(MyError);
    EXPECT_STREQ(err.file, "");
    EXPECT_EQ(err.line, -1);
}

TEST(CoreErrorHandlerTest, DoNothingMacroIsValidStatement)
{
    int side_effect = 0;
    CASTLE_DO_NOTHING;
    side_effect = 1;
    EXPECT_EQ(side_effect, 1);
}

// In the default build (no CASTLE_LOG_ERRORS / CASTLE_USE_ASSERT_FUNCTION),
// castle::error_handler is intentionally NOT defined. We only assert that the
// exception facility above is available; triggering CASTLE_ASSERT_* here would
// abort via the C assert() fallback, so those paths are deliberately untested.

} // namespace
