#include "castle_ext/data/data_pool.hpp"

#include "castle/patterns/observer.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <stdint.h>
#include <thread>

namespace
{
struct imu
{
    float accel_z = 0.0F;
};

struct gnss
{
    double latitude = 0.0;
};

struct battery
{
    uint16_t millivolts = 0U;
};

using pool_type = castle_ext::data::data_pool<2U, imu, castle_ext::data::data_slot<gnss, 3U>, battery>;
using set_type = pool_type::update_set_type;

// Records every notification it receives.
class recording_observer CASTLE_FINAL : public pool_type::observer_type
{
public:
    void notify(const set_type& updates) override
    {
        last = updates;
        ++count;
    }

    set_type last{};
    uint32_t count = 0U;
};
} // namespace

TEST(DataPoolTest, EveryTypeStartsWithDefaultValueAndGenerationZero)
{
    const pool_type pool;
    EXPECT_EQ(pool.get_snapshot<imu>().generation(), 0U);
    EXPECT_EQ(pool.get_snapshot<gnss>()->latitude, 0.0);
    EXPECT_EQ(pool.get_snapshot<battery>()->millivolts, 0U);
}

TEST(DataPoolTest, PublishStoresValuePerTypeIndependently)
{
    pool_type pool;
    EXPECT_EQ(pool.publish(imu{9.81F}), castle::status::ok);
    EXPECT_EQ(pool.publish(battery{3700U}), castle::status::ok);
    EXPECT_EQ(pool.publish(battery{3650U}), castle::status::ok);

    const pool_type& reader = pool;
    EXPECT_FLOAT_EQ(reader.get_snapshot<imu>()->accel_z, 9.81F);
    EXPECT_EQ(reader.get_snapshot<imu>().generation(), 1U);
    EXPECT_EQ(reader.get_snapshot<battery>()->millivolts, 3650U);
    EXPECT_EQ(reader.get_snapshot<battery>().generation(), 2U);
    EXPECT_EQ(reader.get_snapshot<gnss>().generation(), 0U);
}

TEST(DataPoolTest, NotifiesEveryObserverWithOnlyThePublishedType)
{
    pool_type pool;
    recording_observer first;
    pool_type::inbox_type second;
    ASSERT_TRUE(pool.add_observer(&first));
    ASSERT_TRUE(pool.add_observer(&second));

    ASSERT_EQ(pool.publish(gnss{48.1}), castle::status::ok);
    EXPECT_EQ(first.count, 1U);
    EXPECT_EQ(first.last, set_type::of<gnss>());

    ASSERT_EQ(pool.publish(imu{1.0F}), castle::status::ok);
    EXPECT_EQ(first.count, 2U);
    EXPECT_EQ(first.last, set_type::of<imu>());
    EXPECT_EQ(second.take(), (set_type::of<gnss, imu>()));

    ASSERT_TRUE(pool.remove_observer(&first));
    ASSERT_EQ(pool.publish(battery{1U}), castle::status::ok);
    EXPECT_EQ(first.count, 2U);
    EXPECT_EQ(second.take(), set_type::of<battery>());
}

TEST(DataPoolTest, ObserverRegistrationRulesFollowObservable)
{
    pool_type pool;
    recording_observer a;
    recording_observer b;
    recording_observer c;

    EXPECT_FALSE(pool.add_observer(nullptr));
    EXPECT_TRUE(pool.add_observer(&a));
    EXPECT_FALSE(pool.add_observer(&a));
    EXPECT_TRUE(pool.add_observer(&b));
    EXPECT_FALSE(pool.add_observer(&c));
    EXPECT_FALSE(pool.remove_observer(&c));
    EXPECT_TRUE(pool.remove_observer(&a));
    EXPECT_TRUE(pool.add_observer(&c));
}

TEST(DataPoolTest, FailedPublishKeepsValueAndDoesNotNotify)
{
    pool_type pool;
    recording_observer observer;
    ASSERT_TRUE(pool.add_observer(&observer));

    // imu uses the default data_slot<imu, 1>: two held old values exhaust its spare buffers.
    castle_ext::data::snapshot<imu> oldest = pool.get_snapshot<imu>();
    ASSERT_EQ(pool.publish(imu{1.0F}), castle::status::ok);
    castle_ext::data::snapshot<imu> older = pool.get_snapshot<imu>();
    ASSERT_EQ(pool.publish(imu{2.0F}), castle::status::ok);
    ASSERT_EQ(observer.count, 2U);

    EXPECT_EQ(pool.publish(imu{3.0F}), castle::status::full);
    EXPECT_EQ(observer.count, 2U);
    EXPECT_FLOAT_EQ(pool.get_snapshot<imu>()->accel_z, 2.0F);

    oldest.reset();
    EXPECT_EQ(pool.publish(imu{3.0F}), castle::status::ok);
    EXPECT_EQ(observer.count, 3U);
    EXPECT_FLOAT_EQ(older->accel_z, 1.0F);
}

TEST(DataPoolTest, DataSlotEntrySizesHeldSnapshotsOfThatType)
{
    pool_type pool;
    castle_ext::data::snapshot<gnss> held[4];
    for (uint32_t i = 0U; i < 4U; ++i)
    {
        held[i] = pool.get_snapshot<gnss>();
        ASSERT_EQ(pool.publish(gnss{static_cast<double>(i + 1U)}), castle::status::ok);
    }
    EXPECT_EQ(pool.publish(gnss{99.0}), castle::status::full);
    EXPECT_EQ(held[3]->latitude, 3.0);
}

TEST(DataPoolTest, ProducerToConsumerThreadsDeliverLatestValues)
{
    pool_type pool;
    std::atomic<uint32_t> wakes{0U};
    pool_type::inbox_type inbox{[&wakes]() noexcept
    {
        wakes.fetch_add(1U);
    }};
    ASSERT_TRUE(pool.add_observer(&inbox));

    constexpr uint16_t last_value = 5000U;
    std::thread hal([&pool]()
    {
        for (uint16_t i = 1U; i <= last_value; ++i)
        {
            (void)pool.publish(battery{i});
        }
    });

    uint16_t observed = 0U;
    bool monotonic = true;
    while (observed != last_value)
    {
        if (inbox.take().contains<battery>())
        {
            const uint16_t value = pool.get_snapshot<battery>()->millivolts;
            monotonic = monotonic && (value >= observed);
            observed = value;
        }
    }
    hal.join();

    EXPECT_TRUE(monotonic);
    EXPECT_EQ(pool.get_snapshot<battery>().generation(), last_value);
    EXPECT_GE(wakes.load(), 1U);
}
