#include <gtest/gtest.h>

#include "castle/utility/forward.h"
#include "castle/core/traits.h"

namespace
{

enum class Category
{
    Lvalue,
    Rvalue
};

template <typename T>
Category classify(T&) { return Category::Lvalue; }

template <typename T>
Category classify(T&&) { return Category::Rvalue; }

template <typename T>
Category relay(T&& value)
{
    return classify(castle::forward<T>(value));
}

TEST(ForwardTest, PreservesLvalueCategory)
{
    int value = 1;
    EXPECT_EQ(relay(value), Category::Lvalue);
}

TEST(ForwardTest, PreservesRvalueCategory)
{
    EXPECT_EQ(relay(42), Category::Rvalue);
}

TEST(ForwardTest, LvalueForwardYieldsLvalueReference)
{
    int value = 3;
    static_assert(castle::meta::is_same<decltype(castle::forward<int&>(value)), int&>::value,
                  "forward<T&> must yield an lvalue reference");
    (void)value;
}

TEST(ForwardTest, RvalueForwardYieldsRvalueReference)
{
    int value = 3;
    static_assert(castle::meta::is_same<decltype(castle::forward<int>(value)), int&&>::value,
                  "forward<T> must yield an rvalue reference");
    (void)value;
}

TEST(ForwardTest, IsNoexcept)
{
    int value = 0;
    static_assert(noexcept(castle::forward<int&>(value)), "forward must be noexcept");
    (void)value;
}

TEST(ForwardTest, IsConstexpr)
{
    constexpr int value = 9;
    constexpr int forwarded = castle::forward<const int&>(value);
    static_assert(forwarded == 9, "forward must be usable in constexpr");
    EXPECT_EQ(forwarded, 9);
}

} // namespace
