#include "sample_support.hpp"

#include "castle/sync/shared_mutex.hpp"

struct counting_wait_policy
{
    static void wait() CASTLE_NOEXCEPT
    {
    }
};

int main()
{
    castle::shared_mutex<2> lock;

    CASTLE_SAMPLE_CHECK(lock.try_lock_read());
    CASTLE_SAMPLE_CHECK(lock.try_lock_read());
    CASTLE_SAMPLE_CHECK(!lock.try_lock_read());
    CASTLE_SAMPLE_CHECK(!lock.try_lock_write());
    lock.unlock_read();
    lock.unlock_read();

    CASTLE_SAMPLE_CHECK(lock.try_lock_write());
    CASTLE_SAMPLE_CHECK(!lock.try_lock_read());
    CASTLE_SAMPLE_CHECK(!lock.try_lock_write());
    lock.unlock_write();

    lock.lock_read();
    lock.unlock_read();

    lock.lock_write();
    lock.unlock_write();

    castle::shared_mutex<2, counting_wait_policy> custom_lock;
    CASTLE_SAMPLE_CHECK(custom_lock.try_lock_write());
    custom_lock.unlock_write();

    return 0;
}
