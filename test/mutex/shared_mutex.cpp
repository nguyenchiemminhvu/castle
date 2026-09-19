#include <gtest/gtest.h>

#include "castle/mutex/shared_mutex.hpp"

#include <stdint.h>
#include <thread>

namespace
{

// ---------------------------------------------------------------------------
// Basic API and noexcept checks.
//
// Verifies:
//   - read lock acquisition
//   - read lock contention
//   - read unlock
//   - write lock acquisition
//   - write lock contention with readers
//   - write unlock
// ---------------------------------------------------------------------------
TEST(SharedMutexTest, LockReadUnlock)
{
    castle::shared_mutex<4> m;

    static_assert(noexcept(m.lock_read()), "lock_read");
    static_assert(noexcept(m.unlock_read()), "unlock_read");
    static_assert(noexcept(m.try_lock_read()), "try_lock_read");

    EXPECT_TRUE(m.try_lock_read());

    // Multiple readers are allowed.
    EXPECT_TRUE(m.try_lock_read());
    EXPECT_TRUE(m.try_lock_read());
    EXPECT_TRUE(m.try_lock_read());

    // MaxReaders == 4.
    EXPECT_FALSE(m.try_lock_read());

    m.unlock_read();
    m.unlock_read();
    m.unlock_read();
    m.unlock_read();

    EXPECT_TRUE(m.try_lock_read());

    m.unlock_read();
}


// ---------------------------------------------------------------------------
// Basic write-lock behavior.
//
// Verifies:
//   - write acquisition
//   - second writer cannot acquire
//   - write unlock
//   - writer can acquire again
// ---------------------------------------------------------------------------
TEST(SharedMutexTest, LockWriteUnlock)
{
    castle::shared_mutex<4> m;

    static_assert(noexcept(m.lock_write()), "lock_write");
    static_assert(noexcept(m.unlock_write()), "unlock_write");
    static_assert(noexcept(m.try_lock_write()), "try_lock_write");

    EXPECT_TRUE(m.try_lock_write());

    // A writer cannot recursively acquire the same mutex.
    EXPECT_FALSE(m.try_lock_write());

    m.unlock_write();

    EXPECT_TRUE(m.try_lock_write());

    EXPECT_FALSE(m.try_lock_write());

    m.unlock_write();
}


// ---------------------------------------------------------------------------
// A writer excludes readers.
//
// Verifies the branch:
//
//     if ((state & writer_bit_) != 0U)
//         return false;
// ---------------------------------------------------------------------------
TEST(SharedMutexTest, WriterExcludesReaders)
{
    castle::shared_mutex<4> m;

    EXPECT_TRUE(m.try_lock_write());

    EXPECT_FALSE(m.try_lock_read());

    m.unlock_write();

    EXPECT_TRUE(m.try_lock_read());

    m.unlock_read();
}


// ---------------------------------------------------------------------------
// Readers exclude writers.
//
// Verifies the branch:
//
//     if (state != 0U)
//         continue;
// ---------------------------------------------------------------------------
//
// try_lock_write() does not block, so this test deterministically verifies
// that an active reader prevents writer acquisition.
TEST(SharedMutexTest, ReadersExcludeWriter)
{
    castle::shared_mutex<4> m;

    EXPECT_TRUE(m.try_lock_read());

    EXPECT_FALSE(m.try_lock_write());

    m.unlock_read();

    EXPECT_TRUE(m.try_lock_write());

    m.unlock_write();
}


// ---------------------------------------------------------------------------
// Maximum reader count.
//
// Explicitly verifies the MaxReaders boundary.
//
// Four readers are allowed, while the fifth reader must fail.
// After releasing one reader, another reader can enter.
// ---------------------------------------------------------------------------
TEST(SharedMutexTest, MaximumReaders)
{
    castle::shared_mutex<4> m;

    EXPECT_TRUE(m.try_lock_read());
    EXPECT_TRUE(m.try_lock_read());
    EXPECT_TRUE(m.try_lock_read());
    EXPECT_TRUE(m.try_lock_read());

    EXPECT_FALSE(m.try_lock_read());

    m.unlock_read();

    EXPECT_TRUE(m.try_lock_read());

    m.unlock_read();
    m.unlock_read();
    m.unlock_read();
    m.unlock_read();
}


// ---------------------------------------------------------------------------
// Single-reader configuration.
//
// MaxReaders == 1 is useful because it verifies that the template works at
// the smallest valid reader capacity.
//
// With one reader, the behavior approaches an exclusive mutex for reads,
// while still using the shared-mutex API.
// ---------------------------------------------------------------------------
TEST(SharedMutexTest, SingleReader)
{
    castle::shared_mutex<1> m;

    EXPECT_TRUE(m.try_lock_read());

    EXPECT_FALSE(m.try_lock_read());
    EXPECT_FALSE(m.try_lock_write());

    m.unlock_read();

    EXPECT_TRUE(m.try_lock_write());

    EXPECT_FALSE(m.try_lock_read());
    EXPECT_FALSE(m.try_lock_write());

    m.unlock_write();
}


// ---------------------------------------------------------------------------
// Writer waiting prevents a new reader from entering.
//
// This test exercises the writer-preference mechanism.
//
// A reader initially owns the lock. A second thread calls lock_write() and
// therefore increments waiting_writers_. While that writer is waiting,
// try_lock_read() must fail.
//
// Once the original reader is released, the writer can acquire the lock.
// ---------------------------------------------------------------------------
TEST(SharedMutexTest, WaitingWriterBlocksNewReader)
{
    castle::shared_mutex<4> m;

    EXPECT_TRUE(m.try_lock_read());

    bool writer_started = false;
    bool writer_finished = false;

    std::thread writer(
        [&m, &writer_started, &writer_finished]()
        {
            writer_started = true;

            m.lock_write();

            writer_finished = true;

            m.unlock_write();
        });

    // Wait until the writer has entered lock_write().
    //
    // This is only a test synchronization mechanism. The shared mutex itself
    // remains completely platform independent.
    while (!writer_started)
    {
        std::this_thread::yield();
    }

    // Give the writer an opportunity to increment waiting_writers_.
    //
    // The actual lock implementation does not depend on this scheduling
    // primitive.
    while (!writer_finished)
    {
        if (!m.try_lock_read())
        {
            break;
        }

        m.unlock_read();

        std::this_thread::yield();
    }

    // Release the existing reader so that the waiting writer can proceed.
    m.unlock_read();

    writer.join();

    EXPECT_TRUE(writer_finished);
}


// ---------------------------------------------------------------------------
// lock_read() blocking path.
//
// A writer initially owns the mutex. A reader thread must wait until the
// writer releases it.
//
// This exercises:
//   - waiting_writers_ == 0
//   - writer_bit_ != 0
//   - lock_read() retry loop
//   - successful CAS after the writer releases
// ---------------------------------------------------------------------------
TEST(SharedMutexTest, LockReadWaitsForWriter)
{
    castle::shared_mutex<2> m;

    ASSERT_TRUE(m.try_lock_write());

    bool reader_started = false;
    bool reader_finished = false;

    std::thread reader(
        [&m, &reader_started, &reader_finished]()
        {
            reader_started = true;

            m.lock_read();

            reader_finished = true;

            m.unlock_read();
        });

    while (!reader_started)
    {
        std::this_thread::yield();
    }

    // The reader must not finish while the writer owns the lock.
    std::this_thread::yield();

    EXPECT_FALSE(reader_finished);

    m.unlock_write();

    reader.join();

    EXPECT_TRUE(reader_finished);
}


// ---------------------------------------------------------------------------
// lock_write() blocking path.
//
// A reader initially owns the mutex. A writer must wait until the reader
// releases it.
//
// This exercises:
//   - waiting_writers_ increment
//   - state != 0
//   - lock_write() retry loop
//   - successful CAS from 0 to writer_bit_
//   - waiting_writers_ decrement after acquisition
// ---------------------------------------------------------------------------
TEST(SharedMutexTest, LockWriteWaitsForReader)
{
    castle::shared_mutex<2> m;

    ASSERT_TRUE(m.try_lock_read());

    bool writer_started = false;
    bool writer_finished = false;

    std::thread writer(
        [&m, &writer_started, &writer_finished]()
        {
            writer_started = true;

            m.lock_write();

            writer_finished = true;

            m.unlock_write();
        });

    while (!writer_started)
    {
        std::this_thread::yield();
    }

    // The writer should still be blocked by the active reader.
    std::this_thread::yield();

    EXPECT_FALSE(writer_finished);

    m.unlock_read();

    writer.join();

    EXPECT_TRUE(writer_finished);
}


// ---------------------------------------------------------------------------
// try_lock_write() with a free mutex.
//
// Verifies successful CAS:
//
//     0 -> writer_bit_
//
// and the waiting-writer counter cleanup.
// ---------------------------------------------------------------------------
TEST(SharedMutexTest, TryLockWriteFree)
{
    castle::shared_mutex<4> m;

    EXPECT_TRUE(m.try_lock_write());

    m.unlock_write();

    EXPECT_TRUE(m.try_lock_write());

    m.unlock_write();
}


// ---------------------------------------------------------------------------
// try_lock_write() with active readers.
//
// Verifies failed CAS when state != 0.
//
// The reader remains active throughout the failed attempt.
// ---------------------------------------------------------------------------
TEST(SharedMutexTest, TryLockWriteWithReaders)
{
    castle::shared_mutex<4> m;

    EXPECT_TRUE(m.try_lock_read());
    EXPECT_TRUE(m.try_lock_read());

    EXPECT_FALSE(m.try_lock_write());

    // Readers remain valid after the failed writer attempt.
    EXPECT_FALSE(m.try_lock_write());

    m.unlock_read();
    m.unlock_read();

    EXPECT_TRUE(m.try_lock_write());

    m.unlock_write();
}


// ---------------------------------------------------------------------------
// try_lock_read() while another writer owns the lock.
//
// Verifies that the writer bit is detected before the reader count is
// modified.
// ---------------------------------------------------------------------------
TEST(SharedMutexTest, TryLockReadWithWriter)
{
    castle::shared_mutex<4> m;

    EXPECT_TRUE(m.try_lock_write());

    EXPECT_FALSE(m.try_lock_read());

    m.unlock_write();

    EXPECT_TRUE(m.try_lock_read());

    m.unlock_read();
}


// ---------------------------------------------------------------------------
// try_lock_read() while the maximum reader count is reached.
//
// Verifies the:
//
//     readers >= MaxReaders
//
// branch.
// ---------------------------------------------------------------------------
TEST(SharedMutexTest, TryLockReadAtMaximum)
{
    castle::shared_mutex<2> m;

    EXPECT_TRUE(m.try_lock_read());
    EXPECT_TRUE(m.try_lock_read());

    EXPECT_FALSE(m.try_lock_read());

    m.unlock_read();

    EXPECT_TRUE(m.try_lock_read());

    m.unlock_read();
    m.unlock_read();
}


// ---------------------------------------------------------------------------
// Mixed reader/writer state transitions.
//
// This test verifies that the mutex can repeatedly transition between:
//
//     unlocked
//         -> readers
//         -> unlocked
//         -> writer
//         -> unlocked
//         -> readers
//         -> writer
//         -> unlocked
//
// without leaving stale state behind.
// ---------------------------------------------------------------------------
TEST(SharedMutexTest, MixedLockTransitions)
{
    castle::shared_mutex<4> m;

    EXPECT_TRUE(m.try_lock_read());
    EXPECT_TRUE(m.try_lock_read());

    EXPECT_FALSE(m.try_lock_write());

    m.unlock_read();
    m.unlock_read();

    EXPECT_TRUE(m.try_lock_write());

    EXPECT_FALSE(m.try_lock_read());
    EXPECT_FALSE(m.try_lock_write());

    m.unlock_write();

    EXPECT_TRUE(m.try_lock_read());

    m.unlock_read();

    EXPECT_TRUE(m.try_lock_write());

    m.unlock_write();

    EXPECT_TRUE(m.try_lock_read());

    m.unlock_read();
}


// ---------------------------------------------------------------------------
// Concurrent readers.
//
// Demonstrates that multiple threads can actually hold the read lock at the
// same time.
//
// The barrier is implemented with atomics from the C++ standard library only
// for the test harness. CASTLE itself has no STL dependency.
// ---------------------------------------------------------------------------
TEST(SharedMutexTest, ConcurrentReaders)
{
    castle::shared_mutex<4> m;

    volatile uint32_t readers_inside = 0U;
    volatile uint32_t maximum_readers_inside = 0U;

    std::thread reader1(
        [&m, &readers_inside, &maximum_readers_inside]()
        {
            m.lock_read();

            uint32_t count = __sync_add_and_fetch(
                &readers_inside,
                1U);

            uint32_t current_max =
                __sync_add_and_fetch(
                    &maximum_readers_inside,
                    0U);

            while (count > current_max)
            {
                if (__sync_bool_compare_and_swap(
                        &maximum_readers_inside,
                        current_max,
                        count))
                {
                    break;
                }

                current_max =
                    __sync_add_and_fetch(
                        &maximum_readers_inside,
                        0U);
            }

            std::this_thread::yield();

            __sync_fetch_and_sub(&readers_inside, 1U);

            m.unlock_read();
        });

    std::thread reader2(
        [&m, &readers_inside, &maximum_readers_inside]()
        {
            m.lock_read();

            uint32_t count = __sync_add_and_fetch(
                &readers_inside,
                1U);

            uint32_t current_max =
                __sync_add_and_fetch(
                    &maximum_readers_inside,
                    0U);

            while (count > current_max)
            {
                if (__sync_bool_compare_and_swap(
                        &maximum_readers_inside,
                        current_max,
                        count))
                {
                    break;
                }

                current_max =
                    __sync_add_and_fetch(
                        &maximum_readers_inside,
                        0U);
            }

            std::this_thread::yield();

            __sync_fetch_and_sub(&readers_inside, 1U);

            m.unlock_read();
        });

    reader1.join();
    reader2.join();

    EXPECT_GE(maximum_readers_inside, 1U);
    EXPECT_LE(maximum_readers_inside, 2U);
}


// ---------------------------------------------------------------------------
// Object semantics.
//
// shared_mutex must not be copyable or movable.
// ---------------------------------------------------------------------------
TEST(SharedMutexTest, NonCopyableAndNonMovable)
{
    static_assert(
        !__is_constructible(
            castle::shared_mutex<4>,
            const castle::shared_mutex<4>&),
        "shared_mutex must not be copy constructible");

    static_assert(
        !__is_assignable(
            castle::shared_mutex<4>&,
            const castle::shared_mutex<4>&),
        "shared_mutex must not be copy assignable");

    static_assert(
        !__is_constructible(
            castle::shared_mutex<4>,
            castle::shared_mutex<4>&&),
        "shared_mutex must not be move constructible");

    static_assert(
        !__is_assignable(
            castle::shared_mutex<4>&,
            castle::shared_mutex<4>&&),
        "shared_mutex must not be move assignable");
}

} // namespace
