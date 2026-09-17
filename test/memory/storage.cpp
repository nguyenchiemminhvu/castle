#include <gtest/gtest.h>

#include "castle/memory/storage.h"
#include "castle/memory/new.h"

#include <cstdint>

namespace
{

int g_live = 0;

struct Counter
{
    int value;
    Counter() CASTLE_NOEXCEPT : value(0) { ++g_live; }
    explicit Counter(int v) CASTLE_NOEXCEPT : value(v) { ++g_live; }
    ~Counter() { --g_live; }
};

TEST(RawStorageTest, ExposesValueTypeAndCapacity)
{
    using Storage = castle::memory::raw_storage<Counter, 5>;
    static_assert(Storage::capacity == 5U, "capacity constant");
    static_assert(sizeof(Storage::value_type) == sizeof(Counter), "value_type");
    SUCCEED();
}

TEST(RawStorageTest, BytesPointerNonNullAndAligned)
{
    castle::memory::raw_storage<double, 4> storage;
    ASSERT_NE(storage.bytes(), nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(storage.bytes()) % alignof(double), 0U);
}

TEST(RawStorageTest, AddressMatchesBytesPlusOffset)
{
    castle::memory::raw_storage<Counter, 3> storage;
    uint8_t* base = storage.bytes();
    EXPECT_EQ(storage.address(0), static_cast<void*>(base));
    EXPECT_EQ(storage.address(1), static_cast<void*>(base + sizeof(Counter)));
    EXPECT_EQ(storage.address(2), static_cast<void*>(base + 2U * sizeof(Counter)));
}

TEST(RawStorageTest, LifetimeManagedByCaller)
{
    g_live = 0;
    castle::memory::raw_storage<Counter, 4> storage;
    EXPECT_EQ(g_live, 0); // storage alone starts no lifetimes

    for (size_t i = 0; i < 4; ++i)
    {
        ::new (storage.address(i)) Counter(static_cast<int>(i) * 10);
    }
    EXPECT_EQ(g_live, 4);
    EXPECT_EQ(reinterpret_cast<Counter*>(storage.address(3))->value, 30);

    for (size_t i = 0; i < 4; ++i)
    {
        reinterpret_cast<Counter*>(storage.address(i))->~Counter();
    }
    EXPECT_EQ(g_live, 0);
}

TEST(RawStorageTest, ConstAccessors)
{
    castle::memory::raw_storage<int, 2> storage;
    CASTLE_CONST auto& cref = storage;
    EXPECT_EQ(cref.address(0), storage.address(0));
    EXPECT_EQ(cref.bytes(), storage.bytes());
}

TEST(RawStorageTest, ZeroCapacitySpecializationYieldsNull)
{
    castle::memory::raw_storage<int, 0> storage;
    static_assert(castle::memory::raw_storage<int, 0>::capacity == 0U, "zero capacity");
    EXPECT_EQ(storage.address(0), nullptr);
    EXPECT_EQ(storage.bytes(), nullptr);

    CASTLE_CONST auto& cref = storage;
    EXPECT_EQ(cref.address(0), nullptr);
    EXPECT_EQ(cref.bytes(), nullptr);
}

} // namespace
