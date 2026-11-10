#include "castle_ext/data/update_dispatcher.hpp"

#include "castle_ext/data/data_pool.hpp"

#include "castle/patterns/observer.hpp"

#include <gtest/gtest.h>

#include <stdint.h>

namespace
{
struct imu
{
    float accel_z = 0.0F;
};

struct gnss
{
};

struct battery
{
};

using set_type = castle_ext::data::update_set<imu, gnss, battery>;
using dispatcher_type = castle_ext::data::update_dispatcher<imu, gnss, battery>;
} // namespace

TEST(UpdateDispatcherTest, CallsOnlyBoundCallbacksOfNotifiedTypes)
{
    uint32_t imu_calls = 0U;
    uint32_t gnss_calls = 0U;
    dispatcher_type dispatcher;
    dispatcher.bind<imu>([&imu_calls]() noexcept
    {
        ++imu_calls;
    });
    dispatcher.bind<gnss>([&gnss_calls]() noexcept
    {
        ++gnss_calls;
    });

    dispatcher.notify(set_type::of<imu>());
    EXPECT_EQ(imu_calls, 1U);
    EXPECT_EQ(gnss_calls, 0U);

    // battery has no callback: ignored, the bound types of the same set still run.
    dispatcher.notify(set_type::of<gnss, battery>());
    EXPECT_EQ(imu_calls, 1U);
    EXPECT_EQ(gnss_calls, 1U);

    dispatcher.notify(set_type{});
    EXPECT_EQ(imu_calls, 1U);
    EXPECT_EQ(gnss_calls, 1U);
}

TEST(UpdateDispatcherTest, MultiTypeSetRunsCallbacksInDeclarationOrder)
{
    char order[4]{};
    uint32_t count = 0U;
    dispatcher_type dispatcher;
    dispatcher.bind<battery>([&]() noexcept
    {
        order[count++] = 'b';
    });
    dispatcher.bind<imu>([&]() noexcept
    {
        order[count++] = 'i';
    });
    dispatcher.bind<gnss>([&]() noexcept
    {
        order[count++] = 'g';
    });

    dispatcher.notify(set_type::all());
    ASSERT_EQ(count, 3U);
    EXPECT_EQ(order[0], 'i');
    EXPECT_EQ(order[1], 'g');
    EXPECT_EQ(order[2], 'b');
}

TEST(UpdateDispatcherTest, EveryNotificationIsDeliveredWithoutCoalescing)
{
    uint32_t calls = 0U;
    dispatcher_type dispatcher;
    dispatcher.bind<imu>([&calls]() noexcept
    {
        ++calls;
    });

    for (uint32_t i = 0U; i < 5U; ++i)
    {
        dispatcher.notify(set_type::of<imu>());
    }
    EXPECT_EQ(calls, 5U);
}

TEST(UpdateDispatcherTest, BindReplacesAndNullptrClears)
{
    uint32_t first = 0U;
    uint32_t second = 0U;
    dispatcher_type dispatcher;
    EXPECT_FALSE(dispatcher.is_bound<imu>());

    dispatcher.bind<imu>([&first]() noexcept
    {
        ++first;
    });
    EXPECT_TRUE(dispatcher.is_bound<imu>());
    EXPECT_FALSE(dispatcher.is_bound<gnss>());

    dispatcher.bind<imu>([&second]() noexcept
    {
        ++second;
    });
    dispatcher.notify(set_type::of<imu>());
    EXPECT_EQ(first, 0U);
    EXPECT_EQ(second, 1U);

    dispatcher.bind<imu>(nullptr);
    EXPECT_FALSE(dispatcher.is_bound<imu>());
    dispatcher.notify(set_type::of<imu>());
    EXPECT_EQ(second, 1U);
}

TEST(UpdateDispatcherTest, ReceivesNotificationsAsObserver)
{
    uint32_t calls = 0U;
    dispatcher_type dispatcher;
    dispatcher.bind<gnss>([&calls]() noexcept
    {
        ++calls;
    });

    castle::patterns::observable<castle::patterns::observer<set_type>, 1U> source;
    ASSERT_TRUE(source.add_observer(&dispatcher));
    source.notify_observers(set_type::of<gnss>());
    EXPECT_EQ(calls, 1U);
}

TEST(UpdateDispatcherTest, PoolCallbackReadsTheValueJustPublished)
{
    using pool_type = castle_ext::data::data_pool<1U, imu, gnss, battery>;
    pool_type pool;
    pool_type::dispatcher_type dispatcher;

    float seen = 0.0F;
    uint32_t seen_generation = 0U;
    dispatcher.bind<imu>([&pool, &seen, &seen_generation]() noexcept
    {
        const castle_ext::data::snapshot<imu> sample = pool.get_snapshot<imu>();
        seen = sample->accel_z;
        seen_generation = sample.generation();
    });
    ASSERT_TRUE(pool.add_observer(&dispatcher));

    ASSERT_EQ(pool.publish(imu{9.81F}), castle::status::ok);
    EXPECT_FLOAT_EQ(seen, 9.81F);
    EXPECT_EQ(seen_generation, 1U);

    ASSERT_EQ(pool.publish(gnss{}), castle::status::ok);
    EXPECT_EQ(seen_generation, 1U);
}
