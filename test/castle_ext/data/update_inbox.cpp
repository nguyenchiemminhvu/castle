#include "castle_ext/data/update_inbox.hpp"

#include "castle/design_patterns/observer.hpp"
#include "castle/utility/integral_sequence.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <stdint.h>
#include <thread>

namespace
{
struct imu
{
};

struct gnss
{
};

struct battery
{
};

using set_type = castle_ext::data::update_set<imu, gnss, battery>;
using inbox_type = castle_ext::data::update_inbox<imu, gnss, battery>;

template <castle::size_type N>
struct tag
{
};

template <castle::size_type... Is>
castle_ext::data::update_inbox<tag<Is>...> make_tag_inbox(castle::sequence::index_sequence<Is...>);

// 40 types: spans two 32-bit storage words.
using wide_inbox = decltype(make_tag_inbox(castle::sequence::make_index_sequence_t<40U>{}));
using wide_set = wide_inbox::set_type;
} // namespace

TEST(UpdateInboxTest, StartsIdle)
{
    inbox_type inbox;
    EXPECT_FALSE(inbox.has_pending());
    EXPECT_TRUE(inbox.take().empty());
}

TEST(UpdateInboxTest, TakeReturnsCoalescedUpdatesAndClearsThem)
{
    inbox_type inbox;
    inbox.notify(set_type::of<imu>());
    inbox.notify(set_type::of<imu, battery>());
    EXPECT_TRUE(inbox.has_pending());

    EXPECT_EQ(inbox.take(), (set_type::of<imu, battery>()));
    EXPECT_FALSE(inbox.has_pending());
    EXPECT_TRUE(inbox.take().empty());
}

TEST(UpdateInboxTest, EmptyNotificationNeitherPendsNorWakes)
{
    uint32_t wakes = 0U;
    inbox_type inbox{[&wakes]() noexcept
    {
        ++wakes;
    }};

    inbox.notify(set_type{});
    EXPECT_FALSE(inbox.has_pending());
    EXPECT_EQ(wakes, 0U);
}

TEST(UpdateInboxTest, WakesOnlyWhenTurningFromIdleToPending)
{
    uint32_t wakes = 0U;
    inbox_type inbox{[&wakes]() noexcept
    {
        ++wakes;
    }};

    inbox.notify(set_type::of<imu>());
    EXPECT_EQ(wakes, 1U);
    inbox.notify(set_type::of<gnss>());
    inbox.notify(set_type::of<imu>());
    EXPECT_EQ(wakes, 1U);

    EXPECT_EQ(inbox.take(), (set_type::of<imu, gnss>()));
    inbox.notify(set_type::of<battery>());
    EXPECT_EQ(wakes, 2U);
}

TEST(UpdateInboxTest, SetWakeBindsHookAfterConstruction)
{
    uint32_t wakes = 0U;
    inbox_type inbox;
    inbox.notify(set_type::of<imu>());
    EXPECT_EQ(wakes, 0U);

    inbox.take();
    inbox.set_wake([&wakes]() noexcept
    {
        ++wakes;
    });
    inbox.notify(set_type::of<gnss>());
    EXPECT_EQ(wakes, 1U);

    inbox.set_wake(nullptr);
    inbox.take();
    inbox.notify(set_type::of<battery>());
    EXPECT_EQ(wakes, 1U);
}

TEST(UpdateInboxTest, WideInboxNeverMissesAWakeAcrossStorageWords)
{
    uint32_t wakes = 0U;
    wide_inbox inbox{[&wakes]() noexcept
    {
        ++wakes;
    }};

    inbox.notify(wide_set::of<tag<1U>>());
    EXPECT_EQ(wakes, 1U);
    inbox.notify(wide_set::of<tag<2U>>());
    EXPECT_EQ(wakes, 1U);

    // A second storage word turning non-empty wakes again (documented, harmless extra wake).
    inbox.notify(wide_set::of<tag<35U>>());
    EXPECT_EQ(wakes, 2U);
    EXPECT_TRUE(inbox.has_pending());

    EXPECT_EQ(inbox.take(), (wide_set::of<tag<1U>, tag<2U>, tag<35U>>()));
    EXPECT_FALSE(inbox.has_pending());

    // Pending detected in the upper word alone.
    inbox.notify(wide_set::of<tag<39U>>());
    EXPECT_TRUE(inbox.has_pending());
    EXPECT_EQ(wakes, 3U);
}

TEST(UpdateInboxTest, ReceivesNotificationsAsObserver)
{
    inbox_type first;
    inbox_type second;
    castle::design_patterns::observable<castle::design_patterns::observer<set_type>, 2U> source;
    ASSERT_TRUE(source.add_observer(&first));
    ASSERT_TRUE(source.add_observer(&second));

    source.notify_observers(set_type::of<gnss>());
    EXPECT_EQ(first.take(), set_type::of<gnss>());
    EXPECT_EQ(second.take(), set_type::of<gnss>());
}

TEST(UpdateInboxTest, ConcurrentProducersLoseNoUpdate)
{
    std::atomic<uint32_t> wakes{0U};
    inbox_type inbox{[&wakes]() noexcept
    {
        wakes.fetch_add(1U);
    }};

    set_type collected;
    std::atomic<bool> done{false};
    std::thread consumer([&]()
    {
        while (!done.load())
        {
            collected |= inbox.take();
        }
        collected |= inbox.take();
    });

    std::thread imu_producer([&inbox]()
    {
        for (uint32_t i = 0U; i < 10000U; ++i)
        {
            inbox.notify(set_type::of<imu>());
        }
    });
    std::thread gnss_producer([&inbox]()
    {
        for (uint32_t i = 0U; i < 10000U; ++i)
        {
            inbox.notify(set_type::of<gnss, battery>());
        }
    });

    imu_producer.join();
    gnss_producer.join();
    done.store(true);
    consumer.join();

    EXPECT_EQ(collected, set_type::all());
    EXPECT_GE(wakes.load(), 1U);
    EXPECT_FALSE(inbox.has_pending());
}
