#include <gtest/gtest.h>

#include "castle/bit/flags.h"

#include <cstdint>

namespace cb = castle::bit;

namespace
{

using Flags8 = cb::flags<uint8_t>;

CASTLE_CONSTEXPR uint8_t BIT_A = 0x01;
CASTLE_CONSTEXPR uint8_t BIT_B = 0x02;
CASTLE_CONSTEXPR uint8_t BIT_C = 0x04;

// constexpr helpers exercising the constexpr mutating API.
constexpr uint8_t make_set_runtime()
{
    Flags8 f;
    f.set(static_cast<uint8_t>(BIT_A | BIT_C));
    return f.value();
}

constexpr uint8_t make_set_bool()
{
    Flags8 f;
    f.set(BIT_A, true);
    f.set(BIT_B, true);
    f.set(BIT_A, false);
    return f.value();
}

// ── construction ────────────────────────────────────────────────

TEST(Flags, DefaultConstructIsClear)
{
    Flags8 f;
    EXPECT_EQ(f.value(), uint8_t{0x00});
    EXPECT_TRUE(f.none());
    static_assert(Flags8{}.value() == 0u, "default clear");
}

TEST(Flags, PatternConstruct)
{
    Flags8 f{0x05};
    EXPECT_EQ(f.value(), uint8_t{0x05});
    static_assert(Flags8{0x05}.value() == 0x05u, "pattern ctor");
}

TEST(Flags, CopyAndMoveConstruct)
{
    Flags8 a{0x0A};
    Flags8 b{a};
    EXPECT_EQ(b.value(), uint8_t{0x0A});
    Flags8 c{cb::flags<uint8_t>{0x0C}};
    EXPECT_EQ(c.value(), uint8_t{0x0C});
}

TEST(Flags, StaticMembers)
{
    EXPECT_EQ(Flags8::ALL_SET, uint8_t{0xFF});
    EXPECT_EQ(Flags8::ALL_CLEAR, uint8_t{0x00});
    EXPECT_EQ(Flags8::NBITS, static_cast<size_t>(8));
}

// ── test ────────────────────────────────────────────────────────

TEST(Flags, Test)
{
    Flags8 f{0x05};
    EXPECT_TRUE(f.test(BIT_A));
    EXPECT_FALSE(f.test(BIT_B));
    EXPECT_TRUE(f.test(BIT_C));
    EXPECT_TRUE((f.test<BIT_A>()));
    EXPECT_FALSE((f.test<BIT_B>()));
    static_assert(Flags8{0x05}.test(BIT_A), "test A");
    static_assert(Flags8{0x05}.test<BIT_C>(), "test C tpl");
}

// ── set ─────────────────────────────────────────────────────────

TEST(Flags, SetRuntime)
{
    Flags8 f;
    f.set(BIT_A);
    EXPECT_EQ(f.value(), uint8_t{0x01});
    f.set(BIT_C);
    EXPECT_EQ(f.value(), uint8_t{0x05});
    static_assert(make_set_runtime() == 0x05u, "set runtime constexpr");
}

TEST(Flags, SetWithBool)
{
    Flags8 f;
    f.set(BIT_A, true);
    EXPECT_TRUE(f.test(BIT_A));
    f.set(BIT_A, false);
    EXPECT_FALSE(f.test(BIT_A));
    static_assert(make_set_bool() == BIT_B, "set bool constexpr");
}

TEST(Flags, SetTemplate)
{
    Flags8 f;
    f.set<BIT_A>();
    EXPECT_EQ(f.value(), uint8_t{0x01});
    f.set<BIT_B>(true);
    EXPECT_EQ(f.value(), uint8_t{0x03});
    f.set<BIT_B, false>();
    EXPECT_EQ(f.value(), uint8_t{0x01});
}

// ── clear / reset ───────────────────────────────────────────────

TEST(Flags, ClearAll)
{
    Flags8 f{0xFF};
    f.clear();
    EXPECT_EQ(f.value(), uint8_t{0x00});
    EXPECT_TRUE(f.none());
}

TEST(Flags, Reset)
{
    Flags8 f{0x07};
    f.reset(BIT_B);
    EXPECT_EQ(f.value(), uint8_t{0x05});
    f.reset<BIT_A>();
    EXPECT_EQ(f.value(), uint8_t{0x04});
}

// ── flip ────────────────────────────────────────────────────────

TEST(Flags, FlipAll)
{
    Flags8 f{0x0F};
    f.flip();
    EXPECT_EQ(f.value(), uint8_t{0xF0});
}

TEST(Flags, FlipPattern)
{
    Flags8 f{0x05};
    f.flip(BIT_A);
    EXPECT_EQ(f.value(), uint8_t{0x04});
    f.flip<BIT_B>();
    EXPECT_EQ(f.value(), uint8_t{0x06});
}

// ── all / any / none ────────────────────────────────────────────

TEST(Flags, AllAnyNone)
{
    Flags8 empty;
    EXPECT_TRUE(empty.none());
    EXPECT_FALSE(empty.any());
    EXPECT_FALSE(empty.all());

    Flags8 full{0xFF};
    EXPECT_TRUE(full.all());
    EXPECT_TRUE(full.any());
    EXPECT_FALSE(full.none());

    Flags8 some{0x05};
    EXPECT_FALSE(some.all());
    EXPECT_TRUE(some.any());
    EXPECT_FALSE(some.none());
    static_assert(Flags8{0xFF}.all(), "all set");
    static_assert(Flags8{}.none(), "none set");
}

TEST(Flags, AllOf)
{
    Flags8 f{0x05};
    EXPECT_TRUE(f.all_of(BIT_A));
    EXPECT_TRUE(f.all_of(static_cast<uint8_t>(BIT_A | BIT_C)));
    EXPECT_FALSE(f.all_of(static_cast<uint8_t>(BIT_A | BIT_B)));
    EXPECT_TRUE((f.all_of<BIT_C>()));
    EXPECT_FALSE((f.all_of<BIT_B>()));
    static_assert(Flags8{0x05}.all_of(BIT_A), "all_of A");
}

TEST(Flags, AnyOf)
{
    Flags8 f{0x05};
    EXPECT_TRUE(f.any_of(static_cast<uint8_t>(BIT_A | BIT_B)));
    EXPECT_FALSE(f.any_of(BIT_B));
    EXPECT_TRUE((f.any_of<BIT_A>()));
    EXPECT_FALSE((f.any_of<BIT_B>()));
    static_assert(Flags8{0x05}.any_of(BIT_A), "any_of A");
}

TEST(Flags, NoneOf)
{
    Flags8 f{0x05};
    EXPECT_TRUE(f.none_of(BIT_B));
    EXPECT_FALSE(f.none_of(BIT_A));
    EXPECT_TRUE((f.none_of<BIT_B>()));
    EXPECT_FALSE((f.none_of<BIT_C>()));
    static_assert(Flags8{0x05}.none_of(BIT_B), "none_of B");
}

// ── value / conversion ──────────────────────────────────────────

TEST(Flags, ValueGetSet)
{
    Flags8 f;
    f.value(0x0C);
    EXPECT_EQ(f.value(), uint8_t{0x0C});
    const uint8_t raw = f; // implicit conversion operator
    EXPECT_EQ(raw, uint8_t{0x0C});
    static_assert(static_cast<uint8_t>(Flags8{0x0C}) == 0x0Cu, "conversion");
}

// ── compound operators ──────────────────────────────────────────

TEST(Flags, CompoundOr)
{
    Flags8 f{0x01};
    f |= BIT_B;
    EXPECT_EQ(f.value(), uint8_t{0x03});
}

TEST(Flags, CompoundAnd)
{
    Flags8 f{0x07};
    f &= BIT_A;
    EXPECT_EQ(f.value(), uint8_t{0x01});
}

TEST(Flags, CompoundXor)
{
    Flags8 f{0x05};
    f ^= BIT_A;
    EXPECT_EQ(f.value(), uint8_t{0x04});
    f ^= BIT_A;
    EXPECT_EQ(f.value(), uint8_t{0x05});
}

// ── assignment ──────────────────────────────────────────────────

TEST(Flags, Assignment)
{
    Flags8 a{0x0A};
    Flags8 b;
    b = a;
    EXPECT_EQ(b.value(), uint8_t{0x0A});
    b = uint8_t{0x03};
    EXPECT_EQ(b.value(), uint8_t{0x03});
}

// ── swap ────────────────────────────────────────────────────────

TEST(Flags, MemberSwap)
{
    Flags8 a{0x01};
    Flags8 b{0x02};
    a.swap(b);
    EXPECT_EQ(a.value(), uint8_t{0x02});
    EXPECT_EQ(b.value(), uint8_t{0x01});
}

TEST(Flags, FreeSwap)
{
    Flags8 a{0x0F};
    Flags8 b{0xF0};
    cb::swap(a, b);
    EXPECT_EQ(a.value(), uint8_t{0xF0});
    EXPECT_EQ(b.value(), uint8_t{0x0F});
}

// ── relational operators ────────────────────────────────────────

TEST(Flags, EqualityOperators)
{
    Flags8 a{0x05};
    Flags8 b{0x05};
    Flags8 c{0x06};
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
    EXPECT_FALSE(a != b);
    static_assert(Flags8{0x05} == Flags8{0x05}, "eq");
    static_assert(Flags8{0x05} != Flags8{0x06}, "neq");
}

// ── MASK behaviour ──────────────────────────────────────────────

TEST(Flags, MaskRestrictsBits)
{
    using Nibble = cb::flags<uint8_t, 0x0F>;
    EXPECT_EQ(Nibble::ALL_SET, uint8_t{0x0F});

    Nibble f{0xFF}; // high nibble masked away
    EXPECT_EQ(f.value(), uint8_t{0x0F});
    EXPECT_TRUE(f.all());

    Nibble g;
    g.set(0xF0); // outside mask -> ignored
    EXPECT_EQ(g.value(), uint8_t{0x00});
    g.set(0x0C);
    EXPECT_EQ(g.value(), uint8_t{0x0C});
}

TEST(Flags, Widths)
{
    cb::flags<uint16_t> f16{0xFFFF};
    EXPECT_TRUE(f16.all());
    EXPECT_EQ(cb::flags<uint16_t>::NBITS, static_cast<size_t>(16));

    cb::flags<uint32_t> f32{0x80000000U};
    EXPECT_TRUE(f32.test(0x80000000U));
    EXPECT_EQ(cb::flags<uint32_t>::NBITS, static_cast<size_t>(32));

    cb::flags<uint64_t> f64{0x8000000000000000ULL};
    EXPECT_TRUE(f64.any());
    EXPECT_EQ(cb::flags<uint64_t>::NBITS, static_cast<size_t>(64));
}

} // namespace
