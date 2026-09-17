#include <gtest/gtest.h>

#include "castle/memory/static_storage.h"
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

TEST(StaticStorageTest, ExposesCapacityAndValueType)
{
    using Storage = castle::memory::static_storage<Counter, 4>;
    static_assert(Storage::capacity == 4U, "capacity");
    static_assert(sizeof(Storage::value_type) == sizeof(Counter), "value_type");
    SUCCEED();
}

TEST(StaticStorageTest, AddressesAreDistinctAndSpacedBySizeof)
{
    castle::memory::static_storage<Counter, 3> storage;
    CASTLE_CONST uint8_t* a0 = reinterpret_cast<CASTLE_CONST uint8_t*>(storage.address(0));
    CASTLE_CONST uint8_t* a1 = reinterpret_cast<CASTLE_CONST uint8_t*>(storage.address(1));
    CASTLE_CONST uint8_t* a2 = reinterpret_cast<CASTLE_CONST uint8_t*>(storage.address(2));

    EXPECT_EQ(static_cast<size_t>(a1 - a0), sizeof(Counter));
    EXPECT_EQ(static_cast<size_t>(a2 - a1), sizeof(Counter));
}

TEST(StaticStorageTest, BytesPointerIsAlignedForType)
{
    castle::memory::static_storage<double, 2> storage;
    ASSERT_NE(storage.bytes(), nullptr);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(storage.bytes()) % alignof(double), 0U);
}

TEST(StaticStorageTest, ConstructAndDestroyThroughStorage)
{
    g_live = 0;
    castle::memory::static_storage<Counter, 3> storage;

    for (size_t i = 0; i < 3; ++i)
    {
        ::new (storage.address(i)) Counter(static_cast<int>(i) + 1);
    }
    EXPECT_EQ(g_live, 3);

    Counter* first = reinterpret_cast<Counter*>(storage.address(0));
    EXPECT_EQ(first->value, 1);
    EXPECT_EQ(reinterpret_cast<Counter*>(storage.address(2))->value, 3);

    for (size_t i = 0; i < 3; ++i)
    {
        reinterpret_cast<Counter*>(storage.address(i))->~Counter();
    }
    EXPECT_EQ(g_live, 0);
}

TEST(StaticStorageTest, ConstAddressAccessor)
{
    castle::memory::static_storage<int, 2> storage;
    CASTLE_CONST auto& cref = storage;
    EXPECT_EQ(cref.address(0), storage.address(0));
}

} // namespace
