#include "sample_support.hpp"

#include "castle/sync/mutex.hpp"

struct counting_wait_policy
{
    static void wait() CASTLE_NOEXCEPT
    {
    }
};

int main()
{
    castle::mutex lock;

    CASTLE_SAMPLE_CHECK(lock.try_lock());
    CASTLE_SAMPLE_CHECK(!lock.try_lock());
    lock.unlock();

    lock.lock();
    CASTLE_SAMPLE_CHECK(!lock.try_lock());
    lock.unlock();

    castle::basic_mutex<counting_wait_policy> custom_lock;
    CASTLE_SAMPLE_CHECK(custom_lock.try_lock());
    custom_lock.unlock();

    return 0;
}
