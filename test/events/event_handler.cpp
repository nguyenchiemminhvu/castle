/**
 * @brief Unit tests for castle::events::event_handler.
 */

#include "castle/events/event_handler.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>
#include <type_traits>

namespace
{
namespace mock
{

enum class operation : uint8_t
{
    none,
    mutex_init,
    condattr_init,
    condattr_setclock,
    cond_init,
    pthread_create,
    mutex_lock,
    cond_wait,
    cond_timedwait,
    cond_signal,
    cond_broadcast,
    pthread_join,
    clock_gettime
};

std::atomic<int> armed_operation(static_cast<int>(operation::none));
std::atomic<int> remaining_failures(0);
std::atomic<int> clock_failures(0);
std::atomic<int> clock_mode(0);

void arm(operation value, int count = 1)
{
    remaining_failures.store(count);
    armed_operation.store(static_cast<int>(value));
}

void disarm()
{
    remaining_failures.store(0);
    armed_operation.store(static_cast<int>(operation::none));
}

bool consume(operation value)
{
    if (armed_operation.load() != static_cast<int>(value))
    {
        return false;
    }

    int expected = remaining_failures.load();
    while (expected > 0)
    {
        if (remaining_failures.compare_exchange_weak(expected, expected - 1))
        {
            if (expected == 1)
            {
                armed_operation.store(static_cast<int>(operation::none));
            }
            return true;
        }
    }

    return false;
}

void reset()
{
    disarm();
    clock_failures.store(0);
    clock_mode.store(0);
}

} // namespace mock
} // namespace

extern "C"
{

int __real_pthread_mutex_destroy(pthread_mutex_t*);
int __real_pthread_mutex_lock(pthread_mutex_t*);
int __real_pthread_mutex_unlock(pthread_mutex_t*);
int __real_pthread_mutex_init(pthread_mutex_t*, const pthread_mutexattr_t*);
int __real_pthread_condattr_init(pthread_condattr_t*);
int __real_pthread_condattr_destroy(pthread_condattr_t*);
int __real_pthread_condattr_setclock(pthread_condattr_t*, clockid_t);
int __real_pthread_cond_init(pthread_cond_t*, const pthread_condattr_t*);
int __real_pthread_cond_destroy(pthread_cond_t*);
int __real_pthread_cond_wait(pthread_cond_t*, pthread_mutex_t*);
int __real_pthread_cond_timedwait(pthread_cond_t*, pthread_mutex_t*, const struct timespec*);
int __real_pthread_cond_signal(pthread_cond_t*);
int __real_pthread_cond_broadcast(pthread_cond_t*);
int __real_pthread_create(pthread_t*, const pthread_attr_t*, void* (*)(void*), void*);
int __real_pthread_join(pthread_t, void**);
int __real_clock_gettime(clockid_t, struct timespec*);

int __wrap_pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr)
{
    if (mock::consume(mock::operation::mutex_init))
    {
        return EAGAIN;
    }
    return __real_pthread_mutex_init(mutex, attr);
}

int __wrap_pthread_mutex_destroy(pthread_mutex_t* mutex)
{
    return __real_pthread_mutex_destroy(mutex);
}

int __wrap_pthread_mutex_lock(pthread_mutex_t* mutex)
{
    if (mock::consume(mock::operation::mutex_lock))
    {
        return EBUSY;
    }
    return __real_pthread_mutex_lock(mutex);
}

int __wrap_pthread_mutex_unlock(pthread_mutex_t* mutex)
{
    return __real_pthread_mutex_unlock(mutex);
}

int __wrap_pthread_condattr_init(pthread_condattr_t* attr)
{
    if (mock::consume(mock::operation::condattr_init))
    {
        return EAGAIN;
    }
    return __real_pthread_condattr_init(attr);
}

int __wrap_pthread_condattr_destroy(pthread_condattr_t* attr)
{
    return __real_pthread_condattr_destroy(attr);
}

int __wrap_pthread_condattr_setclock(pthread_condattr_t* attr, clockid_t clock_id)
{
    if (mock::consume(mock::operation::condattr_setclock))
    {
        return EINVAL;
    }
    return __real_pthread_condattr_setclock(attr, clock_id);
}

int __wrap_pthread_cond_init(pthread_cond_t* cond, const pthread_condattr_t* attr)
{
    if (mock::consume(mock::operation::cond_init))
    {
        return EAGAIN;
    }
    return __real_pthread_cond_init(cond, attr);
}

int __wrap_pthread_cond_destroy(pthread_cond_t* cond)
{
    return __real_pthread_cond_destroy(cond);
}

int __wrap_pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex)
{
    if (mock::consume(mock::operation::cond_wait))
    {
        return EIO;
    }
    return __real_pthread_cond_wait(cond, mutex);
}

int __wrap_pthread_cond_timedwait(
    pthread_cond_t* cond,
    pthread_mutex_t* mutex,
    const struct timespec* abstime)
{
    if (mock::consume(mock::operation::cond_timedwait))
    {
        return EIO;
    }
    return __real_pthread_cond_timedwait(cond, mutex, abstime);
}

int __wrap_pthread_cond_signal(pthread_cond_t* cond)
{
    if (mock::consume(mock::operation::cond_signal))
    {
        return EIO;
    }
    return __real_pthread_cond_signal(cond);
}

int __wrap_pthread_cond_broadcast(pthread_cond_t* cond)
{
    if (mock::consume(mock::operation::cond_broadcast))
    {
        return EIO;
    }
    return __real_pthread_cond_broadcast(cond);
}

int __wrap_pthread_create(
    pthread_t* thread,
    const pthread_attr_t* attr,
    void* (*start_routine)(void*),
    void* arg)
{
    if (mock::consume(mock::operation::pthread_create))
    {
        return EAGAIN;
    }
    return __real_pthread_create(thread, attr, start_routine, arg);
}

int __wrap_pthread_join(pthread_t thread, void** retval)
{
    int result = __real_pthread_join(thread, retval);
    if (mock::consume(mock::operation::pthread_join))
    {
        return EIO;
    }
    return result;
}

int __wrap_clock_gettime(clockid_t clock_id, struct timespec* ts)
{
    if (clock_id == CLOCK_MONOTONIC)
    {
        if (mock::clock_mode.load() == 1)
        {
            ts->tv_sec = 0;
            ts->tv_nsec = 0;
            return 0;
        }

        if (mock::clock_mode.load() == 2 && mock::clock_failures.load() > 0)
        {
            mock::clock_failures.fetch_sub(1);
            ts->tv_sec = -1;
            ts->tv_nsec = 0;
            return 0;
        }
    }

    return __real_clock_gettime(clock_id, ts);
}

} // extern "C"

namespace
{

bool wait_until_not_running(castle::events::event_handler<2U>& handler)
{
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(300U);
    while (handler.running() && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::yield();
    }
    return !handler.running();
}

TEST(EventHandlerPosixMockTest, MutexInitializationFailureLeavesHandlerUnconfigured)
{
    mock::reset();
    mock::arm(mock::operation::mutex_init);

    castle::events::event_handler<2U> handler;

    EXPECT_FALSE(handler.initialized());
    EXPECT_FALSE(handler.running());
    EXPECT_EQ(handler.post_event([] {}), castle::status::not_configured);
    mock::reset();
}

TEST(EventHandlerPosixMockTest, ConditionAttributeInitializationFailureIsHandled)
{
    mock::reset();
    mock::arm(mock::operation::condattr_init);

    castle::events::event_handler<2U> handler;

    EXPECT_FALSE(handler.initialized());
    EXPECT_FALSE(handler.running());
    mock::reset();
}

TEST(EventHandlerPosixMockTest, ConditionAttributeSetClockFailureIsHandled)
{
    mock::reset();
    mock::arm(mock::operation::condattr_setclock);

    castle::events::event_handler<2U> handler;

    EXPECT_FALSE(handler.initialized());
    EXPECT_FALSE(handler.running());
    mock::reset();
}

TEST(EventHandlerPosixMockTest, ConditionInitializationFailureIsHandled)
{
    mock::reset();
    mock::arm(mock::operation::cond_init);

    castle::events::event_handler<2U> handler;

    EXPECT_FALSE(handler.initialized());
    EXPECT_FALSE(handler.running());
    mock::reset();
}

TEST(EventHandlerPosixMockTest, ThreadCreationFailureIsHandled)
{
    mock::reset();
    mock::arm(mock::operation::pthread_create);

    castle::events::event_handler<2U> handler;

    EXPECT_FALSE(handler.initialized());
    EXPECT_FALSE(handler.running());
    EXPECT_EQ(handler.available(), 0U);
    mock::reset();
}

TEST(EventHandlerPosixMockTest, QueryLockFailureReturnsSafeFallbacks)
{
    mock::reset();
    castle::events::event_handler<2U> handler;

    mock::arm(mock::operation::mutex_lock);
    EXPECT_FALSE(handler.running());
    mock::arm(mock::operation::mutex_lock);
    EXPECT_EQ(handler.pending(), 0U);
    mock::arm(mock::operation::mutex_lock);
    EXPECT_EQ(handler.available(), 0U);

    EXPECT_EQ(handler.shutdown(), castle::status::ok);
    mock::reset();
}

TEST(EventHandlerPosixMockTest, PostLockFailureReturnsSystemCallError)
{
    mock::reset();
    castle::events::event_handler<2U> handler;

    mock::arm(mock::operation::mutex_lock);
    EXPECT_EQ(handler.post_event([] {}), castle::status::system_call_error);

    EXPECT_EQ(handler.shutdown(), castle::status::ok);
    mock::reset();
}

TEST(EventHandlerPosixMockTest, ConditionSignalFailureIsReported)
{
    mock::reset();
    castle::events::event_handler<2U> handler;

    mock::arm(mock::operation::cond_signal);
    EXPECT_EQ(handler.post_event([] {}), castle::status::system_call_error);

    EXPECT_EQ(handler.shutdown(), castle::status::ok);
    mock::reset();
}

TEST(EventHandlerPosixMockTest, WorkerConditionWaitFailureStopsWorker)
{
    mock::reset();
    mock::arm(mock::operation::cond_wait);

    castle::events::event_handler<2U> handler;

    EXPECT_TRUE(wait_until_not_running(handler));
    EXPECT_EQ(handler.shutdown(), castle::status::ok);
    mock::reset();
}

TEST(EventHandlerPosixMockTest, WorkerTimedWaitFailureStopsWorker)
{
    mock::reset();
    castle::events::event_handler<2U> handler;

    mock::arm(mock::operation::cond_timedwait);
    EXPECT_EQ(handler.post_delayed_event(1000U, [] {}), castle::status::ok);

    EXPECT_TRUE(wait_until_not_running(handler));
    EXPECT_EQ(handler.shutdown(), castle::status::ok);
    mock::reset();
}

TEST(EventHandlerPosixMockTest, LockFailureAfterCallbackStopsWorker)
{
    mock::reset();
    castle::events::event_handler<2U> handler;
    std::atomic<bool> callback_ran(false);

    EXPECT_EQ(handler.post_event([&callback_ran]() { callback_ran.store(true); }),
              castle::status::ok);

    const auto callback_deadline = std::chrono::steady_clock::now() +
                                   std::chrono::milliseconds(300U);
    while (!callback_ran.load() && std::chrono::steady_clock::now() < callback_deadline)
    {
        std::this_thread::yield();
    }

    ASSERT_TRUE(callback_ran.load());

    // The next worker-side mutex lock occurs after the callback returns.
    mock::arm(mock::operation::mutex_lock);
    EXPECT_TRUE(wait_until_not_running(handler));

    EXPECT_EQ(handler.shutdown(), castle::status::ok);
    mock::reset();
}

TEST(EventHandlerPosixMockTest, TimedWaitCanReportExternalFailureAfterClockTimeoutPath)
{
    mock::reset();
    castle::events::event_handler<2U> handler;

    // The event is valid, but the timed-wait mock forces a non-timeout error.
    mock::arm(mock::operation::cond_timedwait);
    EXPECT_EQ(handler.post_delayed_event(100U, [] {}), castle::status::ok);
    EXPECT_TRUE(wait_until_not_running(handler));

    EXPECT_EQ(handler.shutdown(), castle::status::ok);
    mock::reset();
}

TEST(EventHandlerPosixMockTest, BroadcastFailureIsReturnedAfterWorkerHasExited)
{
    mock::reset();
    castle::events::event_handler<2U> handler;
    std::atomic<bool> callback_ran(false);

    EXPECT_EQ(handler.post_event([&callback_ran]() {
                  callback_ran.store(true);
                  std::this_thread::sleep_for(std::chrono::milliseconds(30U));
              }),
              castle::status::ok);

    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(300U);
    while (!callback_ran.load() && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::yield();
    }
    ASSERT_TRUE(callback_ran.load());

    mock::arm(mock::operation::cond_broadcast);
    EXPECT_EQ(handler.shutdown(), castle::status::system_call_error);
    mock::reset();
}

TEST(EventHandlerPosixMockTest, JoinFailureIsReportedAfterRealJoinCompletes)
{
    mock::reset();
    castle::events::event_handler<2U> handler;

    mock::arm(mock::operation::pthread_join);
    EXPECT_EQ(handler.shutdown(), castle::status::system_call_error);
    EXPECT_FALSE(handler.initialized());
    mock::reset();
}

TEST(EventHandlerPosixMockTest, NegativeMonotonicClockMakesTimespecConversionFail)
{
    mock::reset();
    mock::clock_mode.store(2);
    mock::clock_failures.store(100);

    castle::events::event_handler<2U> handler;
    EXPECT_EQ(handler.post_delayed_event(1U, [] {}), castle::status::ok);
    EXPECT_TRUE(wait_until_not_running(handler));
    EXPECT_EQ(handler.shutdown(), castle::status::ok);
    mock::reset();
}

} // namespace

namespace
{

using handler_type = castle::events::event_handler<8U, 128U>;

class callback_probe
{
public:
    callback_probe()
        : count_(0U)
        , value_(0U)
    {
    }

    void increment()
    {
        ++count_;
    }

    void add(uint32_t value)
    {
        value_.fetch_add(value);
        count_.fetch_add(1U);
    }

    void add_signed(int32_t value)
    {
        value_.fetch_add(static_cast<uint32_t>(value));
        count_.fetch_add(1U);
    }

    bool wait_for_count(uint32_t expected, uint32_t timeout_ms = 500U)
    {
        const auto deadline = std::chrono::steady_clock::now() +
                              std::chrono::milliseconds(timeout_ms);

        while (count_.load() < expected)
        {
            if (std::chrono::steady_clock::now() >= deadline)
            {
                break;
            }

            std::this_thread::yield();
        }

        return count_.load() >= expected;
    }

    uint32_t count() const
    {
        return count_.load();
    }

    uint32_t value() const
    {
        return value_.load();
    }

private:
    std::atomic<uint32_t> count_;
    std::atomic<uint32_t> value_;
};

void free_function_add(callback_probe* probe, uint32_t value)
{
    if (probe != nullptr)
    {
        probe->add(value);
    }
}

struct callable
{
    explicit callable(callback_probe* probe)
        : probe_(probe)
    {
    }

    void operator()(uint32_t value)
    {
        probe_->add(value);
    }

    callback_probe* probe_;
};

class event_handler_test : public ::testing::Test
{
protected:
    void TearDown() override
    {
        // Each test owns its handler and explicitly shuts it down before the
        // fixture is reused.
    }
};

TEST(EventHandlerTest, PublicTypePropertiesAreCorrect)
{
    using handler_type_local = castle::events::event_handler<3U, 96U, 16U>;

    static_assert(handler_type_local::capacity() == 3U, "capacity mismatch");
    static_assert(handler_type_local::callback_storage_size() == 96U,
                  "callback storage size mismatch");
    static_assert(handler_type_local::callback_storage_alignment() == 16U,
                  "callback storage alignment mismatch");

    EXPECT_TRUE(std::is_same<handler_type_local::status_type, castle::status>::value);
}

TEST(EventHandlerTest, UsesWholeSecondsAndRemainderConversion)
{
    castle::events::event_handler<2U> handler;

    // The ordinary GCC clock implementation reports nanosecond ticks through
    // clock_gettime(). With a microsecond duration period, event_handler's
    // to_timespec() must take the period::num == 1 conversion path.
    EXPECT_EQ(handler.post_delayed_event(1U, [] {}), castle::status::ok);
    std::this_thread::sleep_for(std::chrono::milliseconds(20U));

    EXPECT_EQ(handler.shutdown(), castle::status::ok);
}

TEST(EventHandlerTest, DefaultConstructionInitializesWorkerAndCapacity)
{
    handler_type handler;

    EXPECT_TRUE(handler.initialized());
    EXPECT_TRUE(handler.running());
    EXPECT_TRUE(handler.is_running());
    EXPECT_EQ(handler_type::capacity(), 8U);
    EXPECT_EQ(handler_type::callback_storage_size(), 128U);
    EXPECT_EQ(handler.available(), 8U);
    EXPECT_EQ(handler.pending(), 0U);

    EXPECT_EQ(handler.shutdown(), castle::status::ok);
    EXPECT_FALSE(handler.initialized());
    EXPECT_FALSE(handler.running());
    EXPECT_FALSE(handler.is_running());
    EXPECT_EQ(handler.available(), 0U);
    EXPECT_EQ(handler.pending(), 0U);
}

TEST(EventHandlerTest, ShutdownIsIdempotentAndStopIsAlias)
{
    handler_type handler;

    EXPECT_EQ(handler.stop(), castle::status::ok);
    EXPECT_EQ(handler.stop(), castle::status::ok);
    EXPECT_EQ(handler.shutdown(), castle::status::ok);
    EXPECT_FALSE(handler.initialized());
}

TEST(EventHandlerTest, ImmediateEventWithoutArgumentsExecutesAsynchronously)
{
    handler_type handler;
    callback_probe probe;

    EXPECT_EQ(handler.post_event([&probe]() { probe.increment(); }),
              castle::status::ok);
    EXPECT_TRUE(probe.wait_for_count(1U));
    EXPECT_EQ(probe.count(), 1U);
    EXPECT_EQ(handler.available(), 8U);
    EXPECT_EQ(handler.pending(), 0U);
}

TEST(EventHandlerTest, ImmediateEventSupportsVoidArgsSignature)
{
    handler_type handler;
    callback_probe probe;

    EXPECT_EQ(handler.post_event(&free_function_add, &probe, 7U),
              castle::status::ok);
    EXPECT_TRUE(probe.wait_for_count(1U));
    EXPECT_EQ(probe.value(), 7U);
}

TEST(EventHandlerTest, ImmediateEventSupportsCallableWithArguments)
{
    handler_type handler;
    callback_probe probe;
    callable function_object(&probe);

    EXPECT_EQ(handler.post_event(function_object, 5U), castle::status::ok);
    EXPECT_TRUE(probe.wait_for_count(1U));
    EXPECT_EQ(probe.value(), 5U);
}

TEST(EventHandlerTest, DelayedEventDoesNotExecuteBeforeItsDeadline)
{
    handler_type handler;
    callback_probe probe;

    EXPECT_EQ(handler.post_delayed_event(
                  60U,
                  [&probe]() { probe.increment(); }),
              castle::status::ok);

    std::this_thread::sleep_for(std::chrono::milliseconds(10U));
    EXPECT_EQ(probe.count(), 0U);
    EXPECT_TRUE(probe.wait_for_count(1U, 300U));
}

TEST(EventHandlerTest, DelayedEventSupportsVoidArgsSignature)
{
    handler_type handler;
    callback_probe probe;

    EXPECT_EQ(handler.post_delayed_event(
                  15U,
                  &free_function_add,
                  &probe,
                  11U),
              castle::status::ok);
    EXPECT_TRUE(probe.wait_for_count(1U, 300U));
    EXPECT_EQ(probe.value(), 11U);
}

TEST(EventHandlerTest, EarlierDelayedEventReordersExecution)
{
    using order_handler = castle::events::event_handler<4U>;
    order_handler handler;

    std::mutex mutex;
    std::vector<uint32_t> order;

    EXPECT_EQ(handler.post_delayed_event(
                  70U,
                  [&]() {
                      std::lock_guard<std::mutex> lock(mutex);
                      order.push_back(70U);
                  }),
              castle::status::ok);

    EXPECT_EQ(handler.post_delayed_event(
                  15U,
                  [&]() {
                      std::lock_guard<std::mutex> lock(mutex);
                      order.push_back(15U);
                  }),
              castle::status::ok);

    std::this_thread::sleep_for(std::chrono::milliseconds(140U));

    std::lock_guard<std::mutex> lock(mutex);
    ASSERT_EQ(order.size(), 2U);
    EXPECT_EQ(order[0], 15U);
    EXPECT_EQ(order[1], 70U);
}

TEST(EventHandlerTest, SameDeadlineEventsPreservePostingOrder)
{
    using order_handler = castle::events::event_handler<4U>;
    order_handler handler;

    std::mutex mutex;
    std::vector<uint32_t> order;

    EXPECT_EQ(handler.post_delayed_event(
                  30U,
                  [&]() {
                      std::lock_guard<std::mutex> lock(mutex);
                      order.push_back(1U);
                  }),
              castle::status::ok);

    EXPECT_EQ(handler.post_delayed_event(
                  30U,
                  [&]() {
                      std::lock_guard<std::mutex> lock(mutex);
                      order.push_back(2U);
                  }),
              castle::status::ok);

    std::this_thread::sleep_for(std::chrono::milliseconds(100U));

    std::lock_guard<std::mutex> lock(mutex);
    ASSERT_EQ(order.size(), 2U);
    EXPECT_EQ(order[0], 1U);
    EXPECT_EQ(order[1], 2U);
}

TEST(EventHandlerTest, RepeatedEventWithZeroTimesIsIgnored)
{
    handler_type handler;
    callback_probe probe;

    EXPECT_EQ(handler.post_repeated_event(
                  0U,
                  10U,
                  [&probe]() { probe.increment(); }),
              castle::status::ok);

    std::this_thread::sleep_for(std::chrono::milliseconds(25U));
    EXPECT_EQ(probe.count(), 0U);
    EXPECT_EQ(handler.available(), 8U);
    EXPECT_EQ(handler.pending(), 0U);
}

TEST(EventHandlerTest, RepeatedEventExecutesExactNumberOfTimesAndReusesSlot)
{
    handler_type handler;
    callback_probe probe;

    EXPECT_EQ(handler.post_repeated_event(
                  4U,
                  15U,
                  &free_function_add,
                  &probe,
                  3U),
              castle::status::ok);

    EXPECT_TRUE(probe.wait_for_count(4U, 300U));
    EXPECT_EQ(probe.count(), 4U);
    EXPECT_EQ(probe.value(), 12U);
    EXPECT_EQ(handler.available(), 8U);
    EXPECT_EQ(handler.pending(), 0U);
}

TEST(EventHandlerTest, RepeatedEventWithOneTimeDoesNotReschedule)
{
    handler_type handler;
    callback_probe probe;

    EXPECT_EQ(handler.post_repeated_event(
                  1U,
                  10U,
                  &callback_probe::add,
                  &probe,
                  9U),
              castle::status::ok);

    EXPECT_TRUE(probe.wait_for_count(1U));
    std::this_thread::sleep_for(std::chrono::milliseconds(40U));
    EXPECT_EQ(probe.count(), 1U);
    EXPECT_EQ(probe.value(), 9U);
    EXPECT_EQ(handler.available(), 8U);
}

TEST(EventHandlerTest, MemberFunctionImmediateDelayedAndRepeatedOverloadsExecute)
{
    handler_type handler;
    callback_probe probe;

    EXPECT_EQ(handler.post_event(
                  &callback_probe::add,
                  &probe,
                  1U),
              castle::status::ok);
    EXPECT_EQ(handler.post_delayed_event(
                  10U,
                  &callback_probe::add,
                  &probe,
                  2U),
              castle::status::ok);
    EXPECT_EQ(handler.post_repeated_event(
                  2U,
                  15U,
                  &callback_probe::add,
                  &probe,
                  4U),
              castle::status::ok);

    EXPECT_TRUE(probe.wait_for_count(4U, 300U));
    EXPECT_EQ(probe.count(), 4U);
    EXPECT_EQ(probe.value(), 11U);
}

TEST(EventHandlerTest, NullMemberObjectAndMemberFunctionAreRejected)
{
    handler_type handler;

    EXPECT_EQ(handler.post_event(
                  static_cast<void (callback_probe::*)(uint32_t)>(nullptr),
                  static_cast<callback_probe*>(nullptr),
                  1U),
              castle::status::invalid_argument);

    callback_probe probe;
    EXPECT_EQ(handler.post_event(
                  static_cast<void (callback_probe::*)(uint32_t)>(nullptr),
                  &probe,
                  1U),
              castle::status::invalid_argument);

    EXPECT_EQ(handler.post_event(
                  &callback_probe::add,
                  static_cast<callback_probe*>(nullptr),
                  1U),
              castle::status::invalid_argument);

    EXPECT_EQ(handler.post_delayed_event(
                  5U,
                  &callback_probe::add,
                  static_cast<callback_probe*>(nullptr),
                  1U),
              castle::status::invalid_argument);

    EXPECT_EQ(handler.post_repeated_event(
                  2U,
                  5U,
                  &callback_probe::add,
                  static_cast<callback_probe*>(nullptr),
                  1U),
              castle::status::invalid_argument);
}

TEST(EventHandlerTest, EmptyCallbackIsRejected)
{
    handler_type handler;
    handler_type::task_type empty_task;

    EXPECT_FALSE(empty_task);
    EXPECT_EQ(handler.post_event(empty_task), castle::status::invalid_callback);
}

TEST(EventHandlerTest, EventCapacityIsBounded)
{
    using small_handler = castle::events::event_handler<2U>;
    small_handler handler;
    callback_probe probe;

    EXPECT_EQ(handler.post_delayed_event(
                  80U,
                  [&probe]() { probe.increment(); }),
              castle::status::ok);
    EXPECT_EQ(handler.post_delayed_event(
                  80U,
                  [&probe]() { probe.increment(); }),
              castle::status::ok);
    EXPECT_EQ(handler.available(), 0U);
    EXPECT_EQ(handler.pending(), 2U);

    EXPECT_EQ(handler.post_event([&probe]() { probe.increment(); }),
              castle::status::full);

    EXPECT_TRUE(probe.wait_for_count(2U, 300U));
    EXPECT_EQ(handler.available(), 2U);
}

TEST(EventHandlerTest, PostingAfterShutdownReturnsNotConfigured)
{
    handler_type handler;
    callback_probe probe;

    ASSERT_EQ(handler.shutdown(), castle::status::ok);
    EXPECT_EQ(handler.post_event([&probe]() { probe.increment(); }),
              castle::status::not_configured);
    EXPECT_EQ(handler.post_delayed_event(
                  1U,
                  [&probe]() { probe.increment(); }),
              castle::status::not_configured);
    EXPECT_EQ(handler.post_repeated_event(
                  1U,
                  1U,
                  [&probe]() { probe.increment(); }),
              castle::status::not_configured);
}

TEST(EventHandlerTest, ShutdownDiscardsPendingEvents)
{
    handler_type handler;
    callback_probe probe;

    ASSERT_EQ(handler.post_delayed_event(
                  200U,
                  [&probe]() { probe.increment(); }),
              castle::status::ok);
    ASSERT_EQ(handler.pending(), 1U);

    EXPECT_EQ(handler.shutdown(), castle::status::ok);
    std::this_thread::sleep_for(std::chrono::milliseconds(230U));
    EXPECT_EQ(probe.count(), 0U);
}

TEST(EventHandlerTest, CallbackCanPostAnotherEvent)
{
    handler_type handler;
    callback_probe probe;

    EXPECT_EQ(handler.post_event([&]() {
                  probe.increment();
                  EXPECT_EQ(handler.post_event(
                                [&probe]() { probe.increment(); }),
                            castle::status::ok);
              }),
              castle::status::ok);

    EXPECT_TRUE(probe.wait_for_count(2U, 300U));
    EXPECT_EQ(probe.count(), 2U);
}

TEST(EventHandlerTest, CallbackCanRequestSelfShutdownWithoutJoiningItself)
{
    handler_type handler;
    std::atomic<bool> callback_finished(false);
    std::atomic<castle::status> shutdown_status(castle::status::ok);

    EXPECT_EQ(handler.post_event([&]() {
                  shutdown_status.store(handler.shutdown());
                  callback_finished.store(true);
              }),
              castle::status::ok);

    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(300U);
    while (!callback_finished.load() && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::yield();
    }

    EXPECT_TRUE(callback_finished.load());
    EXPECT_EQ(shutdown_status.load(), castle::status::system_call_error);
    EXPECT_EQ(handler.shutdown(), castle::status::ok);
}

TEST(EventHandlerTest, ShutdownWaitsForExecutingCallbackToFinish)
{
    handler_type handler;
    std::atomic<bool> entered(false);
    std::atomic<bool> exited(false);

    EXPECT_EQ(handler.post_event([&]() {
                  entered.store(true);
                  std::this_thread::sleep_for(std::chrono::milliseconds(50U));
                  exited.store(true);
              }),
              castle::status::ok);

    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(300U);
    while (!entered.load() && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::yield();
    }

    ASSERT_TRUE(entered.load());
    EXPECT_EQ(handler.shutdown(), castle::status::ok);
    EXPECT_TRUE(exited.load());
}

} // namespace
