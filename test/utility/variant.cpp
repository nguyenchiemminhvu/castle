#include <gtest/gtest.h>

#include "castle/utility/variant.h"
#include "castle/core/traits.h"

namespace
{

struct Tracker
{
    int moves = 0;
    Tracker() = default;
    Tracker(const Tracker&) = default;
    Tracker(Tracker&& other) noexcept : moves(other.moves + 1) {}
    Tracker& operator=(const Tracker&) = default;
    Tracker& operator=(Tracker&&) = default;
};

TEST(VariantTest, DefaultConstructsFirstAlternative)
{
    castle::variant<int, double> v;
    EXPECT_EQ(v.index(), 0U);
    EXPECT_FALSE(v.valueless_by_exception());
    EXPECT_EQ(castle::get<0>(v), 0);
}

TEST(VariantTest, InPlaceIndexConstruction)
{
    castle::variant<int, double> v(castle::in_place_index<1>, 3.5);
    EXPECT_EQ(v.index(), 1U);
    EXPECT_DOUBLE_EQ(castle::get<1>(v), 3.5);
}

TEST(VariantTest, InPlaceTypeConstruction)
{
    castle::variant<int, double> v(castle::meta::in_place_type<double>, 2.5);
    EXPECT_EQ(v.index(), 1U);
    EXPECT_DOUBLE_EQ(castle::get<double>(v), 2.5);
}

TEST(VariantTest, ConvertingConstructor)
{
    castle::variant<int, double> v(4.0);
    EXPECT_EQ(v.index(), 1U);
    EXPECT_TRUE(castle::holds_alternative<double>(v));
    EXPECT_DOUBLE_EQ(castle::get<double>(v), 4.0);
}

TEST(VariantTest, HoldsAlternative)
{
    castle::variant<int, double> v(7);
    EXPECT_TRUE(castle::holds_alternative<int>(v));
    EXPECT_FALSE(castle::holds_alternative<double>(v));
    EXPECT_TRUE(castle::holds_alternative<0>(v));
    EXPECT_FALSE(castle::holds_alternative<1>(v));
}

TEST(VariantTest, GetByType)
{
    castle::variant<int, double> v(42);
    EXPECT_EQ(castle::get<int>(v), 42);
}

TEST(VariantTest, GetIfReturnsPointerForActive)
{
    castle::variant<int, double> v(9);
    int* p = castle::get_if<int>(&v);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(*p, 9);
}

TEST(VariantTest, GetIfReturnsNullForInactive)
{
    castle::variant<int, double> v(9);
    EXPECT_EQ(castle::get_if<double>(&v), nullptr);
    EXPECT_EQ(castle::get_if<1>(&v), nullptr);
}

TEST(VariantTest, GetIfHandlesNullVariant)
{
    castle::variant<int, double>* null_variant = nullptr;
    EXPECT_EQ(castle::get_if<int>(null_variant), nullptr);
}

TEST(VariantTest, EmplaceByIndex)
{
    castle::variant<int, double> v;
    double& ref = v.emplace<1>(6.5);
    EXPECT_EQ(v.index(), 1U);
    EXPECT_DOUBLE_EQ(ref, 6.5);
}

TEST(VariantTest, EmplaceByType)
{
    castle::variant<int, double> v;
    int& ref = v.emplace<int>(11);
    EXPECT_EQ(v.index(), 0U);
    EXPECT_EQ(ref, 11);
}

TEST(VariantTest, ValueAssignment)
{
    castle::variant<int, double> v;
    v = 3.5;
    EXPECT_EQ(v.index(), 1U);
    EXPECT_DOUBLE_EQ(castle::get<double>(v), 3.5);
    v = 8;
    EXPECT_EQ(v.index(), 0U);
    EXPECT_EQ(castle::get<int>(v), 8);
}

TEST(VariantTest, CopyConstruction)
{
    castle::variant<int, double> source(5);
    castle::variant<int, double> copy(source);
    EXPECT_EQ(copy.index(), 0U);
    EXPECT_EQ(castle::get<int>(copy), 5);
}

TEST(VariantTest, CopyAssignment)
{
    castle::variant<int, double> a(1);
    castle::variant<int, double> b(2.0);
    a = b;
    EXPECT_EQ(a.index(), 1U);
    EXPECT_DOUBLE_EQ(castle::get<double>(a), 2.0);
}

TEST(VariantTest, MoveConstructionMovesActive)
{
    castle::variant<Tracker, int> source;
    castle::variant<Tracker, int> moved(castle::move(source));
    EXPECT_EQ(moved.index(), 0U);
    EXPECT_GE(castle::get<Tracker>(moved).moves, 1);
}

TEST(VariantTest, Swap)
{
    castle::variant<int, double> a(1);
    castle::variant<int, double> b(2.0);
    a.swap(b);
    EXPECT_EQ(a.index(), 1U);
    EXPECT_DOUBLE_EQ(castle::get<double>(a), 2.0);
    EXPECT_EQ(b.index(), 0U);
    EXPECT_EQ(castle::get<int>(b), 1);
}

TEST(VariantTest, FreeSwap)
{
    castle::variant<int, double> a(3);
    castle::variant<int, double> b(9);
    castle::swap(a, b);
    EXPECT_EQ(castle::get<int>(a), 9);
    EXPECT_EQ(castle::get<int>(b), 3);
}

TEST(VariantTest, ResetBecomesValueless)
{
    castle::variant<int, double> v(1);
    v.reset();
    EXPECT_TRUE(v.valueless_by_exception());
    EXPECT_EQ(v.index(), castle::variant_npos);
}

TEST(VariantTest, VisitReturnsValue)
{
    castle::variant<int, double> v(5);
    int result = castle::visit([](auto value) { return static_cast<int>(value); }, v);
    EXPECT_EQ(result, 5);

    v = 2.9;
    result = castle::visit([](auto value) { return static_cast<int>(value); }, v);
    EXPECT_EQ(result, 2);
}

TEST(VariantTest, VisitVoidReturn)
{
    castle::variant<int, double> v(7);
    int captured = 0;
    castle::visit([&captured](auto value) { captured = static_cast<int>(value); }, v);
    EXPECT_EQ(captured, 7);
}

TEST(VariantTest, MakeVariant)
{
    auto v = castle::make_variant<int>(21);
    EXPECT_EQ(v.index(), 0U);
    EXPECT_EQ(castle::get<int>(v), 21);
}

TEST(VariantTest, IsSupportedType)
{
    using variant_type = castle::variant<int, double>;
    static_assert(variant_type::is_supported_type<int>(), "int must be supported");
    static_assert(variant_type::is_supported_type<double>(), "double must be supported");
    static_assert(!variant_type::is_supported_type<char>(), "char must not be supported");
}

TEST(VariantTest, VariantSizeAndAlternative)
{
    using variant_type = castle::variant<int, double, char>;
    static_assert(castle::variant_size<variant_type>::value == 3U, "variant_size must be 3");
    static_assert(castle::variant_size_v<variant_type> == 3U, "variant_size_v must be 3");
    static_assert(castle::meta::is_same<castle::variant_alternative_t<0, variant_type>, int>::value,
                  "alternative 0 must be int");
    static_assert(castle::meta::is_same<castle::variant_alternative_t<1, variant_type>, double>::value,
                  "alternative 1 must be double");
}

TEST(VariantTest, NposValue)
{
    EXPECT_EQ(castle::variant_npos, static_cast<castle::size_type>(-1));
}

} // namespace
