#include <gtest/gtest.h>

#include "castle/utility/tuple.h"
#include "castle/core/traits.h"

namespace
{

TEST(TupleTest, ValueConstructionAndGetByIndex)
{
    castle::tuple<int, float, char> t(10, 3.5f, 'a');
    EXPECT_EQ(castle::get<0>(t), 10);
    EXPECT_FLOAT_EQ(castle::get<1>(t), 3.5f);
    EXPECT_EQ(castle::get<2>(t), 'a');
}

TEST(TupleTest, Size)
{
    using tuple_type = castle::tuple<int, float, char>;
    EXPECT_EQ(tuple_type::size(), 3U);
    static_assert(castle::tuple_size<tuple_type>::value == 3U, "tuple_size must be 3");
    static_assert(castle::tuple_size_v<tuple_type> == 3U, "tuple_size_v must be 3");
}

TEST(TupleTest, TupleElement)
{
    using tuple_type = castle::tuple<int, float, char>;
    static_assert(castle::meta::is_same<castle::tuple_element_t<0, tuple_type>, int>::value,
                  "element 0 must be int");
    static_assert(castle::meta::is_same<castle::tuple_element_t<1, tuple_type>, float>::value,
                  "element 1 must be float");
    static_assert(castle::meta::is_same<castle::tuple_element_t<2, tuple_type>, char>::value,
                  "element 2 must be char");
}

TEST(TupleTest, DefaultConstruction)
{
    castle::tuple<int, int> t;
    EXPECT_EQ(castle::get<0>(t), 0);
    EXPECT_EQ(castle::get<1>(t), 0);
}

TEST(TupleTest, ModifyThroughGet)
{
    castle::tuple<int, int> t(1, 2);
    castle::get<0>(t) = 100;
    castle::get<1>(t) = 200;
    EXPECT_EQ(castle::get<0>(t), 100);
    EXPECT_EQ(castle::get<1>(t), 200);
}

TEST(TupleTest, GetByType)
{
    castle::tuple<int, float, char> t(7, 1.5f, 'x');
    EXPECT_EQ(castle::get<int>(t), 7);
    EXPECT_FLOAT_EQ(castle::get<float>(t), 1.5f);
    EXPECT_EQ(castle::get<char>(t), 'x');
}

TEST(TupleTest, GetByIndexOnConst)
{
    const castle::tuple<int, float> t(4, 2.0f);
    EXPECT_EQ(castle::get<0>(t), 4);
    EXPECT_FLOAT_EQ(castle::get<1>(t), 2.0f);
}

TEST(TupleTest, DuplicateTypesByIndex)
{
    castle::tuple<int, int, int> t(1, 2, 3);
    EXPECT_EQ(castle::get<0>(t), 1);
    EXPECT_EQ(castle::get<1>(t), 2);
    EXPECT_EQ(castle::get<2>(t), 3);
}

TEST(TupleTest, CopyConstruction)
{
    castle::tuple<int, int> a(5, 6);
    castle::tuple<int, int> b(a);
    EXPECT_EQ(castle::get<0>(b), 5);
    EXPECT_EQ(castle::get<1>(b), 6);
}

TEST(TupleTest, CopyAssignment)
{
    castle::tuple<int, int> a(1, 2);
    castle::tuple<int, int> b;
    b = a;
    EXPECT_EQ(castle::get<0>(b), 1);
    EXPECT_EQ(castle::get<1>(b), 2);
}

TEST(TupleTest, MakeTupleDeducesDecayedTypes)
{
    auto t = castle::make_tuple(10, 3.14f, 'c');
    static_assert(castle::meta::is_same<decltype(t), castle::tuple<int, float, char>>::value,
                  "make_tuple must decay argument types");
    EXPECT_EQ(castle::get<0>(t), 10);
    EXPECT_EQ(castle::get<2>(t), 'c');
}

TEST(TupleTest, TieCreatesReferences)
{
    int a = 0;
    float b = 0.0f;
    auto refs = castle::tie(a, b);
    castle::get<0>(refs) = 42;
    castle::get<1>(refs) = 2.5f;
    EXPECT_EQ(a, 42);
    EXPECT_FLOAT_EQ(b, 2.5f);
}

TEST(TupleTest, IgnoreIsAssignmentSink)
{
    castle::ignore = 5;
    castle::ignore = "text";
    SUCCEED();
}

TEST(TupleTest, ForwardAsTupleReferencesSource)
{
    int a = 11;
    int b = 22;
    auto t = castle::forward_as_tuple(a, castle::move(b));
    EXPECT_EQ(castle::get<0>(t), 11);
    EXPECT_EQ(castle::get<1>(t), 22);
}

TEST(TupleTest, ConstexprConstructionAndGet)
{
    constexpr castle::tuple<int, int> t(1, 2);
    static_assert(castle::get<0>(t) == 1, "get must be constexpr");
    static_assert(castle::get<1>(t) == 2, "get must be constexpr");
    static_assert(castle::tuple<int, int>::size() == 2U, "size must be constexpr");
}

} // namespace
