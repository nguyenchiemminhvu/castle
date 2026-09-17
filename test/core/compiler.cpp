#include <gtest/gtest.h>

#include <type_traits>

#include "castle/core/compiler.h"

namespace
{

// Standard-attribute macros must all be visible after including compiler.h.
#if !defined(CASTLE_UNUSED) || !defined(CASTLE_NODISCARD) || !defined(CASTLE_FALL_THROUGH)  \
    || !defined(CASTLE_DEPRECATED) || !defined(CASTLE_DEPRECATED_MSG)
#error "CASTLE standard attribute macros are not defined"
#endif

// Keyword/qualifier macros.
#if !defined(CASTLE_VOLATILE) || !defined(CASTLE_MUTABLE) || !defined(CASTLE_CONST)         \
    || !defined(CASTLE_CONSTEXPR) || !defined(CASTLE_IF_CONSTEXPR) || !defined(CASTLE_NOEXCEPT) \
    || !defined(CASTLE_DEFAULT) || !defined(CASTLE_DELETE) || !defined(CASTLE_OVERRIDE)     \
    || !defined(CASTLE_FINAL) || !defined(CASTLE_VIRTUAL) || !defined(CASTLE_MOVE)          \
    || !defined(CASTLE_FORWARD)
#error "CASTLE keyword macros are not defined"
#endif

// GCC is the active variant on this Linux/x86_64 build.
#if !defined(CASTLE_COMPILER_GCC)
#error "Expected the GCC compiler variant to be selected by compiler.h"
#endif

#if !defined(CASTLE_INLINE) || !defined(CASTLE_HOT) || !defined(CASTLE_COLD)                \
    || !defined(CASTLE_NOINLINE) || !defined(CASTLE_LIKELY) || !defined(CASTLE_UNLIKELY)    \
    || !defined(CASTLE_UNREACHABLE) || !defined(CASTLE_PACKED_ATTR) || !defined(CASTLE_RESTRICT)
#error "CASTLE GCC variant macros are not defined via compiler.h"
#endif

CASTLE_CONSTEXPR int kConstexprValue = 7 * 6;

CASTLE_INLINE int inlined_add(int a, int b) CASTLE_NOEXCEPT { return a + b; }
CASTLE_NOINLINE int noinlined_mul(int a, int b) CASTLE_NOEXCEPT { return a * b; }
CASTLE_HOT int hot_fn(int a) CASTLE_NOEXCEPT { return a + 1; }
CASTLE_COLD int cold_fn(int a) CASTLE_NOEXCEPT { return a - 1; }

CASTLE_NODISCARD int must_use() CASTLE_NOEXCEPT { return 5; }

int restrict_sum(const int* CASTLE_RESTRICT a, const int* CASTLE_RESTRICT b) CASTLE_NOEXCEPT
{
    return *a + *b;
}

struct CASTLE_PACKED_ATTR PackedStruct
{
    unsigned char a;
    int b;
};

struct Base
{
    CASTLE_VIRTUAL int value() CASTLE_CONST CASTLE_NOEXCEPT { return 1; }
    CASTLE_VIRTUAL ~Base() CASTLE_DEFAULT;
};

struct Derived CASTLE_FINAL : Base
{
    int value() CASTLE_CONST CASTLE_NOEXCEPT CASTLE_OVERRIDE { return 2; }
};

struct NonCopyable
{
    NonCopyable() CASTLE_DEFAULT;
    NonCopyable(CASTLE_CONST NonCopyable&) CASTLE_DELETE;
    NonCopyable& operator=(CASTLE_CONST NonCopyable&) CASTLE_DELETE;
};

struct Counter
{
    CASTLE_MUTABLE int calls = 0;
    int touch() CASTLE_CONST CASTLE_NOEXCEPT
    {
        ++calls;
        return calls;
    }
};

int classify(int x) CASTLE_NOEXCEPT
{
    int r = 0;
    switch (x)
    {
    case 0:
        r += 1;
        CASTLE_FALL_THROUGH;
    case 1:
        r += 2;
        break;
    default:
        r = -1;
        break;
    }
    return r;
}

template <typename T>
int pick() CASTLE_NOEXCEPT
{
    CASTLE_IF_CONSTEXPR (sizeof(T) >= 4)
    {
        return 4;
    }
    else
    {
        return 1;
    }
}

TEST(CoreCompilerTest, ConstexprMacroProducesCompileTimeConstant)
{
    static_assert(kConstexprValue == 42, "constexpr macro failed");
    CASTLE_CONSTEXPR int local = kConstexprValue + 1;
    static_assert(local == 43, "local constexpr failed");
    EXPECT_EQ(kConstexprValue, 42);
}

TEST(CoreCompilerTest, AttributedFunctionsBehaveNormally)
{
    EXPECT_EQ(inlined_add(3, 4), 7);
    EXPECT_EQ(noinlined_mul(6, 7), 42);
    EXPECT_EQ(hot_fn(41), 42);
    EXPECT_EQ(cold_fn(1), 0);
    EXPECT_EQ(must_use(), 5);

    const int x = 5;
    const int y = 9;
    EXPECT_EQ(restrict_sum(&x, &y), 14);
}

TEST(CoreCompilerTest, PackedAttributeRemovesPadding)
{
    EXPECT_EQ(sizeof(PackedStruct), sizeof(unsigned char) + sizeof(int));
}

TEST(CoreCompilerTest, VirtualOverrideFinalKeywordsWork)
{
    Derived d;
    Base& b = d;
    EXPECT_EQ(b.value(), 2);
    EXPECT_EQ(Base{}.value(), 1);
}

TEST(CoreCompilerTest, DeleteKeywordDisablesCopy)
{
    static_assert(!std::is_copy_constructible<NonCopyable>::value,
                  "CASTLE_DELETE should suppress copy construction");
    NonCopyable n;
    (void)n;
    SUCCEED();
}

TEST(CoreCompilerTest, MutableAllowsMutationInConstContext)
{
    const Counter c;
    EXPECT_EQ(c.touch(), 1);
    EXPECT_EQ(c.touch(), 2);
}

TEST(CoreCompilerTest, FallThroughSwitchAccumulates)
{
    EXPECT_EQ(classify(0), 3);
    EXPECT_EQ(classify(1), 2);
    EXPECT_EQ(classify(9), -1);
}

TEST(CoreCompilerTest, IfConstexprSelectsBranchBySize)
{
    EXPECT_EQ(pick<int>(), 4);
    EXPECT_EQ(pick<char>(), 1);
}

TEST(CoreCompilerTest, VolatileQualifierMacroCompiles)
{
    CASTLE_VOLATILE int sink = 0;
    sink = 11;
    EXPECT_EQ(sink, 11);
}

} // namespace
