#include <gtest/gtest.h>
#include "castle/callbacks/callback_dispatcher.hpp"
#include "castle/callbacks/function.hpp"

#include <stdint.h>
#include <type_traits>

namespace
{
using callback_type = castle::callbacks::function<void(uint32_t), 64U, 8U>;
using dispatcher_type = castle::callbacks::callback_dispatcher<callback_type>;

struct owner
{
    callback_type first{};
    callback_type second{};
    dispatcher_type dispatcher{};
    uint32_t total = 0U;
};

TEST(CallbackDispatcherTest, IsNotCopyable)
{
    static_assert(!std::is_copy_constructible<dispatcher_type>::value, "copy constructor must be deleted");
    static_assert(!std::is_copy_assignable<dispatcher_type>::value, "copy assignment must be deleted");
}

TEST(CallbackDispatcherTest, InvokesInPlaceKeepingMutableState)
{
    dispatcher_type dispatcher;
    uint32_t observed = 0U;
    callback_type callback{[count = 0U, &observed](uint32_t) mutable noexcept
                           {
                               ++count;
                               observed = count;
                           }};

    dispatcher.invoke(callback, 0U);
    dispatcher.invoke(callback, 0U);
    EXPECT_EQ(observed, 2U);
    EXPECT_FALSE(dispatcher.is_dispatching());
}

TEST(CallbackDispatcherTest, ReplaceAndClearWhileIdleAreImmediate)
{
    dispatcher_type dispatcher;
    uint32_t total = 0U;
    callback_type callback{};

    dispatcher.replace(callback, callback_type{[&total](uint32_t v) noexcept { total += v; }});
    dispatcher.invoke(callback, 5U);
    EXPECT_EQ(total, 5U);

    dispatcher.clear(callback);
    EXPECT_FALSE(static_cast<bool>(callback));
}

TEST(CallbackDispatcherTest, IsDispatchingAndIsRunningReflectInvocation)
{
    owner o{};
    bool dispatching_inside = false;
    bool running_inside = false;
    o.dispatcher.replace(o.first, callback_type{[&o, &dispatching_inside, &running_inside](uint32_t) noexcept
                                                {
                                                    dispatching_inside = o.dispatcher.is_dispatching();
                                                    running_inside = o.dispatcher.is_running(o.first);
                                                }});

    EXPECT_FALSE(o.dispatcher.is_dispatching());
    o.dispatcher.invoke(o.first, 0U);
    EXPECT_TRUE(dispatching_inside);
    EXPECT_TRUE(running_inside);
    EXPECT_FALSE(o.dispatcher.is_dispatching());
    EXPECT_FALSE(o.dispatcher.is_running(o.first));
}

TEST(CallbackDispatcherTest, SelfReplaceIsDeferredUntilReturn)
{
    owner o{};
    o.dispatcher.replace(o.first, callback_type{[&o](uint32_t value) noexcept
                                                {
                                                    o.total += value;
                                                    o.dispatcher.replace(o.first, callback_type{[&o](uint32_t v) noexcept
                                                                                                {
                                                                                                    o.total += (v * 10U);
                                                                                                }});
                                                    EXPECT_EQ(o.total, value);
                                                }});

    o.dispatcher.invoke(o.first, 1U);
    EXPECT_EQ(o.total, 1U);
    EXPECT_TRUE(static_cast<bool>(o.first));

    o.dispatcher.invoke(o.first, 2U);
    EXPECT_EQ(o.total, 21U);
}

TEST(CallbackDispatcherTest, SelfClearIsDeferredUntilReturn)
{
    owner o{};
    o.dispatcher.replace(o.first, callback_type{[&o](uint32_t value) noexcept
                                                {
                                                    o.total += value;
                                                    o.dispatcher.clear(o.first);
                                                    EXPECT_EQ(o.total, value);
                                                    EXPECT_TRUE(static_cast<bool>(o.first));
                                                }});

    o.dispatcher.invoke(o.first, 7U);
    EXPECT_EQ(o.total, 7U);
    EXPECT_FALSE(static_cast<bool>(o.first));
}

TEST(CallbackDispatcherTest, LastRequestDuringCallWins)
{
    owner o{};
    o.dispatcher.replace(o.first, callback_type{[&o](uint32_t) noexcept
                                                {
                                                    o.dispatcher.replace(o.first, callback_type{[&o](uint32_t) noexcept { o.total += 100U; }});
                                                    o.dispatcher.clear(o.first);
                                                }});

    o.dispatcher.invoke(o.first, 0U);
    EXPECT_FALSE(static_cast<bool>(o.first));
    EXPECT_EQ(o.total, 0U);
}

TEST(CallbackDispatcherTest, ReplacingAnotherCallbackDuringCallIsImmediate)
{
    owner o{};
    o.dispatcher.replace(o.second, callback_type{[&o](uint32_t) noexcept { o.total += 1U; }});
    o.dispatcher.replace(o.first, callback_type{[&o](uint32_t) noexcept
                                                {
                                                    EXPECT_FALSE(o.dispatcher.is_running(o.second));
                                                    o.dispatcher.replace(o.second, callback_type{[&o](uint32_t) noexcept { o.total += 100U; }});
                                                }});

    o.dispatcher.invoke(o.first, 0U);
    o.dispatcher.invoke(o.second, 0U);
    EXPECT_EQ(o.total, 100U);
}
} // namespace
