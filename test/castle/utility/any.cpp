#include <gtest/gtest.h>

#include "castle/utility/any.hpp"
#include "castle/utility/type_index.hpp"
#include "castle/memory/alignment.hpp"

#include <stdint.h>

namespace
{

struct Counted
{
    static int live;
    static int copies;
    static int moves;

    int value;

    explicit Counted(int v) : value(v) { ++live; }
    Counted(const Counted& other) : value(other.value) { ++live; ++copies; }
    Counted(Counted&& other) noexcept : value(other.value) { other.value = -1; ++live; ++moves; }
    ~Counted() { --live; }

    static void clear()
    {
        live = 0;
        copies = 0;
        moves = 0;
    }
};

int Counted::live = 0;
int Counted::copies = 0;
int Counted::moves = 0;

struct Pair
{
    int first;
    int second;
    Pair(int a, int b) : first(a), second(b) {}
};

struct Holder
{
    int field;
    char padding[32];
};

struct MoveOnly
{
    MoveOnly() = default;
    MoveOnly(const MoveOnly&) = delete;
    MoveOnly(MoveOnly&&) = default;
};

struct alignas(32) OverAligned
{
    char byte;
};

struct Big
{
    char bytes[castle::any::capacity + 1U];
};

using small_any = castle::basic_any<8U, 4U>;

class AnyTest : public ::testing::Test
{
protected:
    void SetUp() override { Counted::clear(); }
    void TearDown() override { EXPECT_EQ(Counted::live, 0); }
};

TEST_F(AnyTest, TypeReturnsSharedTypeInfo)
{
    castle::any value(1U);
    EXPECT_EQ(&value.type(), &castle::type_id<unsigned>());
    EXPECT_TRUE(value.type() != castle::type_id<int>());
    EXPECT_STREQ(value.type().name(), castle::type_id<unsigned>().name());

    castle::type_index key = value.type();
    EXPECT_TRUE(key == castle::type_id<const unsigned&>());
    value.reset();
    EXPECT_EQ(&value.type(), &castle::type_id<void>());
}

TEST_F(AnyTest, IsSupportedTypeRules)
{
    static_assert(castle::any::is_supported_type<int>(), "");
    static_assert(castle::any::is_supported_type<const Pair>(), "");
    static_assert(!castle::any::is_supported_type<void>(), "");
    static_assert(!castle::any::is_supported_type<int&>(), "");
    static_assert(!castle::any::is_supported_type<void()>(), "");
    static_assert(!castle::any::is_supported_type<int[2]>(), "");
    static_assert(!castle::any::is_supported_type<MoveOnly>(), "");
    static_assert(!castle::any::is_supported_type<castle::any>(), "");
    static_assert(!castle::any::is_supported_type<small_any>(), "");
    static_assert(!castle::any::is_supported_type<Big>(), "");
    static_assert(!castle::any::is_supported_type<OverAligned>(), "");
    static_assert(castle::basic_any<32U, 32U>::is_supported_type<OverAligned>(), "");
    static_assert(!small_any::is_supported_type<uint64_t>(), "");
    EXPECT_TRUE(castle::any::is_supported_type<int>());
    EXPECT_FALSE(castle::any::is_supported_type<Big>());
}

TEST_F(AnyTest, GeometryAndFootprint)
{
    static_assert(castle::any::capacity == castle::inplace_storage_reserved, "");
    static_assert(castle::any::alignment == castle::inplace_alignment_default, "");
    static_assert(small_any::capacity == 8U, "");
    static_assert(small_any::alignment == 4U, "");
    // One pointer of bookkeeping on top of the buffer.
    EXPECT_EQ(sizeof(castle::basic_any<16U, alignof(void*)>), 16U + sizeof(void*));
    EXPECT_EQ(alignof(castle::basic_any<32U, 32U>), 32U);
}

TEST_F(AnyTest, DefaultIsEmpty)
{
    castle::any value;
    EXPECT_FALSE(value.has_value());
    EXPECT_TRUE(value.type() == castle::type_id<void>());
    EXPECT_EQ(castle::any_cast<int>(&value), nullptr);
}

TEST_F(AnyTest, ValueConstructionDecays)
{
    castle::any value(42);
    EXPECT_TRUE(value.has_value());
    EXPECT_TRUE(value.type() == castle::type_id<int>());
    EXPECT_EQ(castle::any_cast<int>(value), 42);

    const char text[] = "abc";
    castle::any pointer(text);
    EXPECT_TRUE(pointer.type() == castle::type_id<const char*>());
    EXPECT_EQ(castle::any_cast<const char*>(pointer), text);

    const Pair source(1, 2);
    castle::any copied(source);
    EXPECT_TRUE(copied.type() == castle::type_id<Pair>());
    EXPECT_EQ(castle::any_cast<Pair&>(copied).second, 2);
}

TEST_F(AnyTest, ValueConstructionForwardsCategory)
{
    Counted source(5);
    castle::any copied(source);
    EXPECT_EQ(Counted::copies, 1);
    EXPECT_EQ(Counted::moves, 0);

    castle::any moved(castle::move(source));
    EXPECT_EQ(Counted::copies, 1);
    EXPECT_EQ(Counted::moves, 1);
    EXPECT_EQ(source.value, -1);
    EXPECT_EQ(castle::any_cast<Counted&>(moved).value, 5);
}

TEST_F(AnyTest, InPlaceConstruction)
{
    castle::any value(castle::meta::in_place_type<Pair>, 3, 4);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(castle::any_cast<Pair&>(value).first, 3);
    EXPECT_EQ(castle::any_cast<Pair&>(value).second, 4);

    castle::any counted(castle::meta::in_place_type<const Counted>, 9);
    EXPECT_TRUE(counted.type() == castle::type_id<Counted>());
    EXPECT_EQ(Counted::copies + Counted::moves, 0);
    EXPECT_EQ(Counted::live, 1);
}

TEST_F(AnyTest, CopyConstruction)
{
    castle::any source(castle::meta::in_place_type<Counted>, 7);
    castle::any copy(source);
    EXPECT_EQ(Counted::copies, 1);
    EXPECT_EQ(Counted::live, 2);
    EXPECT_EQ(castle::any_cast<Counted&>(copy).value, 7);
    EXPECT_EQ(castle::any_cast<Counted&>(source).value, 7);
    EXPECT_NE(castle::any_cast<Counted>(&copy), castle::any_cast<Counted>(&source));

    castle::any empty;
    castle::any empty_copy(empty);
    EXPECT_FALSE(empty_copy.has_value());
}

TEST_F(AnyTest, MoveConstructionRelocatesAndEmptiesSource)
{
    castle::any source(castle::meta::in_place_type<Counted>, 7);
    castle::any moved(castle::move(source));
    EXPECT_FALSE(source.has_value());
    EXPECT_TRUE(source.type() == castle::type_id<void>());
    EXPECT_EQ(Counted::moves, 1);
    EXPECT_EQ(Counted::live, 1);
    EXPECT_EQ(castle::any_cast<Counted&>(moved).value, 7);

    castle::any empty;
    castle::any empty_moved(castle::move(empty));
    EXPECT_FALSE(empty_moved.has_value());
}

TEST_F(AnyTest, CopyAssignment)
{
    castle::any source(castle::meta::in_place_type<Counted>, 1);
    castle::any target(castle::meta::in_place_type<Counted>, 2);
    target = source;
    EXPECT_EQ(castle::any_cast<Counted&>(target).value, 1);
    EXPECT_EQ(castle::any_cast<Counted&>(source).value, 1);
    EXPECT_EQ(Counted::live, 2);

    castle::any& self = target;
    target = self;
    EXPECT_EQ(castle::any_cast<Counted&>(target).value, 1);
    EXPECT_EQ(Counted::live, 2);

    castle::any empty;
    target = empty;
    EXPECT_FALSE(target.has_value());
    EXPECT_EQ(Counted::live, 1);
}

TEST_F(AnyTest, MoveAssignment)
{
    castle::any source(castle::meta::in_place_type<Counted>, 1);
    castle::any target(2.0);
    target = castle::move(source);
    EXPECT_FALSE(source.has_value());
    EXPECT_EQ(castle::any_cast<Counted&>(target).value, 1);
    EXPECT_EQ(Counted::live, 1);

    castle::any& self = target;
    target = castle::move(self);
    EXPECT_TRUE(target.has_value());
    EXPECT_EQ(castle::any_cast<Counted&>(target).value, 1);

    target = castle::any();
    EXPECT_FALSE(target.has_value());
    EXPECT_EQ(Counted::live, 0);
}

TEST_F(AnyTest, ValueAssignmentReplacesType)
{
    castle::any value(castle::meta::in_place_type<Counted>, 1);
    value = 2.5F;
    EXPECT_EQ(Counted::live, 0);
    EXPECT_TRUE(value.type() == castle::type_id<float>());
    EXPECT_EQ(castle::any_cast<float>(value), 2.5F);

    value = Pair(5, 6);
    EXPECT_EQ(castle::any_cast<const Pair&>(value).first, 5);
}

TEST_F(AnyTest, AssignmentFromAliasedSourceIsSafe)
{
    castle::any value(Holder{11, {}});
    value = castle::any_cast<Holder&>(value).field;
    EXPECT_TRUE(value.type() == castle::type_id<int>());
    EXPECT_EQ(castle::any_cast<int>(value), 11);
}

TEST_F(AnyTest, NestedAnyInLargerAny)
{
    struct Node
    {
        castle::any child;
    };
    castle::basic_any<sizeof(Node), alignof(Node)> outer(Node{castle::any(castle::meta::in_place_type<Counted>, 4)});
    EXPECT_EQ(Counted::live, 1);
    castle::any& child = castle::any_cast<Node&>(outer).child;
    EXPECT_EQ(castle::any_cast<Counted&>(child).value, 4);
    outer.reset();
    EXPECT_EQ(Counted::live, 0);
}

TEST_F(AnyTest, EmplaceDestroysPreviousValue)
{
    castle::any value(castle::meta::in_place_type<Counted>, 1);
    Counted& second = value.emplace<Counted>(2);
    EXPECT_EQ(second.value, 2);
    EXPECT_EQ(Counted::live, 1);
    EXPECT_EQ(Counted::copies + Counted::moves, 0);
    EXPECT_EQ(&second, castle::any_cast<Counted>(&value));

    Pair& pair = value.emplace<const Pair>(7, 8);
    EXPECT_EQ(pair.second, 8);
    EXPECT_EQ(Counted::live, 0);
    EXPECT_TRUE(value.type() == castle::type_id<Pair>());
}

TEST_F(AnyTest, ResetDestroysOnceAndIsIdempotent)
{
    castle::any value(castle::meta::in_place_type<Counted>, 1);
    value.reset();
    EXPECT_FALSE(value.has_value());
    EXPECT_EQ(Counted::live, 0);
    value.reset();
    EXPECT_FALSE(value.has_value());
}

TEST_F(AnyTest, SwapAllStateCombinations)
{
    castle::any a(castle::meta::in_place_type<Counted>, 1);
    castle::any b(2.0);
    a.swap(b);
    EXPECT_EQ(castle::any_cast<double>(a), 2.0);
    EXPECT_EQ(castle::any_cast<Counted&>(b).value, 1);

    castle::any empty;
    empty.swap(b);
    EXPECT_FALSE(b.has_value());
    EXPECT_EQ(castle::any_cast<Counted&>(empty).value, 1);

    castle::swap(empty, b);
    EXPECT_FALSE(empty.has_value());
    EXPECT_EQ(castle::any_cast<Counted&>(b).value, 1);

    b.swap(b);
    EXPECT_EQ(castle::any_cast<Counted&>(b).value, 1);
    EXPECT_EQ(Counted::live, 1);

    castle::any none_a;
    castle::any none_b;
    none_a.swap(none_b);
    EXPECT_FALSE(none_a.has_value());
    EXPECT_FALSE(none_b.has_value());
}

TEST_F(AnyTest, PointerCast)
{
    castle::any value(10);
    int* match = castle::any_cast<int>(&value);
    ASSERT_NE(match, nullptr);
    EXPECT_EQ(*match, 10);
    EXPECT_EQ(castle::any_cast<const int>(&value), match);
    EXPECT_EQ(castle::any_cast<long>(&value), nullptr);
    EXPECT_EQ(castle::any_cast<unsigned>(&value), nullptr);

    castle::any* null_any = nullptr;
    EXPECT_EQ(castle::any_cast<int>(null_any), nullptr);

    const castle::any& view = value;
    const int* const_match = castle::any_cast<int>(&view);
    EXPECT_EQ(const_match, match);
    EXPECT_EQ(castle::any_cast<long>(&view), nullptr);

    const castle::any* const_null = nullptr;
    EXPECT_EQ(castle::any_cast<int>(const_null), nullptr);

    const castle::any empty;
    EXPECT_EQ(castle::any_cast<int>(&empty), nullptr);
}

TEST_F(AnyTest, StoredValueIsAligned)
{
    castle::basic_any<32U, 32U> value(OverAligned{'x'});
    OverAligned* stored = castle::any_cast<OverAligned>(&value);
    ASSERT_NE(stored, nullptr);
    EXPECT_TRUE(castle::memory::is_aligned<OverAligned>(stored));
    EXPECT_EQ(stored->byte, 'x');
}

TEST_F(AnyTest, ReferenceAndValueCasts)
{
    castle::any value(castle::meta::in_place_type<Counted>, 3);

    castle::any_cast<Counted&>(value).value = 4;
    EXPECT_EQ(castle::any_cast<const Counted&>(value).value, 4);

    Counted copy = castle::any_cast<Counted>(value);
    EXPECT_EQ(copy.value, 4);
    EXPECT_EQ(Counted::copies, 1);

    const castle::any& view = value;
    EXPECT_EQ(castle::any_cast<const Counted&>(view).value, 4);
    Counted const_copy = castle::any_cast<Counted>(view);
    EXPECT_EQ(const_copy.value, 4);
    EXPECT_EQ(Counted::copies, 2);

    Counted moved = castle::any_cast<Counted>(castle::move(value));
    EXPECT_EQ(moved.value, 4);
    EXPECT_EQ(Counted::moves, 1);
    EXPECT_TRUE(value.has_value());
    EXPECT_EQ(castle::any_cast<Counted&>(value).value, -1);

    Counted&& rvalue = castle::any_cast<Counted&&>(castle::move(value));
    EXPECT_EQ(&rvalue, castle::any_cast<Counted>(&value));
}

TEST_F(AnyTest, MakeAny)
{
    castle::any standard = castle::make_any<Pair>(1, 2);
    EXPECT_TRUE(standard.type() == castle::type_id<Pair>());
    EXPECT_EQ(castle::any_cast<Pair&>(standard).second, 2);

    small_any small = castle::make_any<uint16_t, 8U, 4U>(uint16_t(9U));
    EXPECT_EQ(castle::any_cast<uint16_t>(small), 9U);

    castle::any counted = castle::make_any<Counted>(5);
    EXPECT_EQ(Counted::live, 1);
    EXPECT_EQ(castle::any_cast<Counted&>(counted).value, 5);
}

TEST_F(AnyTest, CustomCapacityRoundTrip)
{
    small_any a(uint32_t(1U));
    small_any b(a);
    b = uint16_t(2U);
    a.swap(b);
    EXPECT_EQ(castle::any_cast<uint16_t>(a), 2U);
    EXPECT_EQ(castle::any_cast<uint32_t>(b), 1U);
}

} // namespace
