#include <gtest/gtest.h>
#include "castle/cache/basic_cache.hpp"
#include "castle/cache/lru.hpp"

#include <vector>

namespace
{
// Every key lands in the same bucket, so chain walking and unlinking at each position is exercised.
struct CollidingHash
{
    castle::size_type operator()(int) const noexcept { return 0U; }
};

// Keys equal when they match modulo 100; the hash honours that equivalence.
struct ModHash
{
    explicit ModHash(castle::size_type salt = 0U) : salt_(salt) {}
    castle::size_type operator()(int key) const noexcept { return static_cast<castle::size_type>(key % 100) + salt_; }
    castle::size_type salt_;
};

struct ModEqual
{
    bool operator()(int lhs, int rhs) const noexcept { return (lhs % 100) == (rhs % 100); }
};

struct Tracker
{
    explicit Tracker(int v = 0) : value(v) { ++live; }
    Tracker(const Tracker& other) : value(other.value) { ++live; }
    Tracker(Tracker&& other) noexcept : value(other.value) { other.value = -1; ++live; }
    Tracker& operator=(const Tracker&) = default;
    Tracker& operator=(Tracker&& other) noexcept { value = other.value; other.value = -1; return *this; }
    ~Tracker() { --live; }
    int value;
    static int live;
};
int Tracker::live = 0;

struct MoveOnly
{
    explicit MoveOnly(int v) : value(v) {}
    MoveOnly(const MoveOnly&) = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;
    MoveOnly(MoveOnly&& other) noexcept : value(other.value) { other.value = -1; }
    MoveOnly& operator=(MoveOnly&& other) noexcept { value = other.value; other.value = -1; return *this; }
    int value;
};

// Policy that records every hook call, then delegates ordering to the LRU state.
struct HookCounts
{
    int resets = 0;
    int inserts = 0;
    int accesses = 0;
    int erases = 0;
};
HookCounts g_hooks;

struct recording_policy
{
    template <castle::size_type N>
    class state
    {
    public:
        void reset() noexcept { ++g_hooks.resets; order_.reset(); }
        void on_insert(castle::size_type slot) noexcept { ++g_hooks.inserts; order_.on_insert(slot); }
        void on_access(castle::size_type slot) noexcept { ++g_hooks.accesses; order_.on_access(slot); }
        void on_erase(castle::size_type slot) noexcept { ++g_hooks.erases; order_.on_erase(slot); }
        castle::size_type victim() const noexcept { return order_.victim(); }

    private:
        typename castle::cache::lru_policy::template state<N> order_;
    };
};

template <typename Cache>
std::vector<int> drain(Cache& cache)
{
    std::vector<int> keys;
    while (cache.evict([&keys](const int& key, auto&) { keys.push_back(key); }) == castle::status::ok)
    {
    }
    return keys;
}

using V = std::vector<int>;

TEST(CacheBasicTest, IndexTypeIsSmallestFittingUnsigned)
{
    static_assert(sizeof(castle::cache::detail::cache_index_t<1U>) == 1U, "1");
    static_assert(sizeof(castle::cache::detail::cache_index_t<255U>) == 1U, "255");
    static_assert(sizeof(castle::cache::detail::cache_index_t<256U>) == 2U, "256");
    static_assert(sizeof(castle::cache::detail::cache_index_t<65535U>) == 2U, "65535");
    static_assert(sizeof(castle::cache::detail::cache_index_t<65536U>) == 4U, "65536");
    EXPECT_EQ(sizeof(castle::cache::detail::cache_index_t<100U>), 1U);
}

TEST(CacheBasicTest, EmptyState)
{
    castle::cache::lru<int, int, 3U> cache;
    static_assert(castle::cache::lru<int, int, 3U>::capacity() == 3U, "capacity");
    EXPECT_EQ(cache.capacity(), 3U);
    EXPECT_EQ(cache.size(), 0U);
    EXPECT_TRUE(cache.empty());
    EXPECT_FALSE(cache.full());
    EXPECT_FALSE(cache.contains(1));
    EXPECT_EQ(cache.get(1), nullptr);
    EXPECT_EQ(cache.peek(1), nullptr);
    EXPECT_EQ(cache.erase(1), castle::status::not_found);
    EXPECT_EQ(cache.evict(), castle::status::empty);
}

TEST(CacheBasicTest, PutInsertsAndUpdatesInPlace)
{
    castle::cache::lru<int, int, 3U> cache;
    int& first = cache.put(1, 10);
    EXPECT_EQ(first, 10);
    EXPECT_EQ(cache.size(), 1U);
    EXPECT_FALSE(cache.empty());

    int& updated = cache.put(1, 11);
    EXPECT_EQ(&updated, &first);
    EXPECT_EQ(first, 11);
    EXPECT_EQ(cache.size(), 1U);

    updated = 12;
    ASSERT_NE(cache.get(1), nullptr);
    EXPECT_EQ(*cache.get(1), 12);
}

TEST(CacheBasicTest, LookupsReturnStoredValues)
{
    castle::cache::lru<int, int, 3U> cache;
    cache.put(1, 10);
    cache.put(2, 20);

    int* hit = cache.get(2);
    ASSERT_NE(hit, nullptr);
    EXPECT_EQ(*hit, 20);

    int* peeked = cache.peek(1);
    ASSERT_NE(peeked, nullptr);
    *peeked = 15;

    const auto& view = cache;
    const int* const_peeked = view.peek(1);
    ASSERT_NE(const_peeked, nullptr);
    EXPECT_EQ(*const_peeked, 15);
    EXPECT_EQ(view.peek(3), nullptr);
    EXPECT_TRUE(view.contains(2));
    EXPECT_FALSE(view.contains(3));
}

TEST(CacheBasicTest, FullCacheEvictsExactlyOneEntryPerNewKey)
{
    castle::cache::lru<int, int, 2U> cache;
    cache.put(1, 10);
    cache.put(2, 20);
    EXPECT_TRUE(cache.full());

    cache.put(3, 30);
    EXPECT_EQ(cache.size(), 2U);
    EXPECT_TRUE(cache.full());
    EXPECT_FALSE(cache.contains(1));
    EXPECT_TRUE(cache.contains(2));
    EXPECT_TRUE(cache.contains(3));
}

TEST(CacheBasicTest, EvictionCallbackSeesVictimBeforeDestruction)
{
    castle::cache::lru<int, Tracker, 2U> cache;
    cache.put(1, Tracker(10));
    const Tracker twenty(20);
    cache.put(2, twenty);

    int calls = 0;
    int evicted_key = 0;
    int evicted_value = 0;
    auto on_evict = [&](const int& key, Tracker& value)
    {
        ++calls;
        evicted_key = key;
        evicted_value = value.value;
    };

    cache.put(2, Tracker(21), on_evict); // update: no eviction
    cache.put(2, twenty, on_evict);      // update by copy: no eviction
    EXPECT_EQ(calls, 0);

    cache.put(3, Tracker(30), on_evict); // move overload evicts key 1
    EXPECT_EQ(calls, 1);
    EXPECT_EQ(evicted_key, 1);
    EXPECT_EQ(evicted_value, 10);

    const Tracker forty(40);
    cache.put(4, forty, on_evict);       // copy overload evicts key 2
    EXPECT_EQ(calls, 2);
    EXPECT_EQ(evicted_key, 2);
    EXPECT_EQ(evicted_value, 20);
    EXPECT_EQ(cache.peek(4)->value, 40);
}

TEST(CacheBasicTest, EvictionCallbackCanMoveValueOut)
{
    castle::cache::lru<int, MoveOnly, 1U> cache;
    cache.put(1, MoveOnly(7));
    int salvaged = 0;
    cache.put(2, MoveOnly(8), [&salvaged](const int&, MoveOnly& value)
    {
        MoveOnly taken(static_cast<MoveOnly&&>(value));
        salvaged = taken.value;
    });
    EXPECT_EQ(salvaged, 7);
    EXPECT_EQ(cache.peek(2)->value, 8);
    cache.put(2, MoveOnly(9));
    EXPECT_EQ(cache.peek(2)->value, 9);
}

TEST(CacheBasicTest, EvictRemovesPolicyVictim)
{
    castle::cache::lru<int, int, 3U> cache;
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);

    EXPECT_EQ(cache.evict(), castle::status::ok);
    EXPECT_EQ(cache.size(), 2U);
    EXPECT_FALSE(cache.contains(1));

    int value = 0;
    EXPECT_EQ(cache.evict([&value](const int&, int& v) { value = v; }), castle::status::ok);
    EXPECT_EQ(value, 20);
    EXPECT_EQ(drain(cache), V({3}));
    EXPECT_TRUE(cache.empty());
    EXPECT_EQ(cache.evict(), castle::status::empty);
}

TEST(CacheBasicTest, EraseFromEveryChainPosition)
{
    castle::cache::basic_cache<int, int, 4U, castle::cache::lru_policy, CollidingHash> cache;
    for (int key = 1; key <= 4; ++key)
    {
        cache.put(key, key * 10);
    }
    // Chain order is newest first: 4 -> 3 -> 2 -> 1.
    EXPECT_EQ(cache.erase(3), castle::status::ok); // middle
    EXPECT_EQ(cache.erase(1), castle::status::ok); // tail
    EXPECT_EQ(cache.erase(4), castle::status::ok); // head
    EXPECT_EQ(cache.erase(4), castle::status::not_found);
    EXPECT_EQ(cache.erase(9), castle::status::not_found);
    EXPECT_EQ(cache.size(), 1U);
    ASSERT_NE(cache.peek(2), nullptr);
    EXPECT_EQ(*cache.peek(2), 20);

    // Freed slots are reused.
    cache.put(5, 50);
    cache.put(6, 60);
    cache.put(7, 70);
    EXPECT_TRUE(cache.full());
    EXPECT_EQ(drain(cache), V({2, 5, 6, 7}));
}

TEST(CacheBasicTest, CollidingKeysEvictCorrectly)
{
    castle::cache::basic_cache<int, int, 3U, castle::cache::lru_policy, CollidingHash> cache;
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);
    cache.get(1);
    cache.put(4, 40); // evicts 2 from the middle of the shared chain
    EXPECT_FALSE(cache.contains(2));
    EXPECT_EQ(*cache.peek(1), 10);
    EXPECT_EQ(*cache.peek(3), 30);
    EXPECT_EQ(*cache.peek(4), 40);
}

TEST(CacheBasicTest, CustomHashAndEqualityAreUsed)
{
    castle::cache::basic_cache<int, int, 4U, castle::cache::lru_policy, ModHash, ModEqual> cache(ModHash(3U), ModEqual());
    cache.put(7, 70);
    EXPECT_TRUE(cache.contains(107));
    cache.put(207, 71);
    EXPECT_EQ(cache.size(), 1U);
    EXPECT_EQ(*cache.get(7), 71);

    castle::cache::basic_cache<int, int, 4U, castle::cache::lru_policy, ModHash, ModEqual> defaulted_equal(ModHash(1U));
    defaulted_equal.put(5, 50);
    EXPECT_TRUE(defaulted_equal.contains(105));
}

TEST(CacheBasicTest, ClearDestroysEntriesAndAllowsReuse)
{
    Tracker::live = 0;
    {
        castle::cache::lru<int, Tracker, 4U> cache;
        cache.put(1, Tracker(1));
        cache.put(2, Tracker(2));
        cache.put(3, Tracker(3));
        EXPECT_EQ(Tracker::live, 3);

        cache.clear();
        EXPECT_EQ(Tracker::live, 0);
        EXPECT_TRUE(cache.empty());
        EXPECT_FALSE(cache.contains(1));

        for (int key = 10; key < 14; ++key)
        {
            cache.put(key, Tracker(key));
        }
        EXPECT_TRUE(cache.full());
        EXPECT_EQ(Tracker::live, 4);
        cache.put(20, Tracker(20));
        EXPECT_EQ(Tracker::live, 4);
        EXPECT_EQ(cache.erase(11), castle::status::ok);
        EXPECT_EQ(Tracker::live, 3);
    }
    EXPECT_EQ(Tracker::live, 0); // destructor releases the remaining entries
}

TEST(CacheBasicTest, LargeCapacityUsesWiderIndex)
{
    static castle::cache::lru<uint16_t, uint8_t, 300U> cache;
    for (uint16_t key = 0U; key < 300U; ++key)
    {
        cache.put(key, static_cast<uint8_t>(key));
    }
    EXPECT_TRUE(cache.full());
    cache.get(0U);
    cache.put(300U, 1U);
    EXPECT_TRUE(cache.contains(0U));
    EXPECT_FALSE(cache.contains(1U));
    EXPECT_EQ(*cache.peek(299U), static_cast<uint8_t>(299U));
}

TEST(CacheBasicTest, PolicyHooksFollowTheContract)
{
    g_hooks = HookCounts();
    castle::cache::basic_cache<int, int, 2U, recording_policy> cache;
    cache.put(1, 10);
    cache.put(2, 20);
    EXPECT_EQ(g_hooks.inserts, 2);
    EXPECT_EQ(g_hooks.accesses, 0);

    cache.put(1, 11); // update
    cache.get(2);     // hit
    cache.get(9);     // miss
    cache.peek(1);
    static_cast<void>(cache.contains(1));
    EXPECT_EQ(g_hooks.accesses, 2);

    cache.put(3, 30); // evicts 1 (least recently used)
    EXPECT_EQ(g_hooks.inserts, 3);
    EXPECT_EQ(g_hooks.erases, 1);
    EXPECT_FALSE(cache.contains(1));

    EXPECT_EQ(cache.erase(2), castle::status::ok);
    EXPECT_EQ(cache.evict(), castle::status::ok);
    EXPECT_EQ(g_hooks.erases, 3);

    cache.clear();
    EXPECT_EQ(g_hooks.resets, 1);
}
}
