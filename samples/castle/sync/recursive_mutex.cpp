#include "sample_support.hpp"

#include "castle/sync/recursive_mutex.hpp"

struct counting_wait_policy
{
    static void wait() CASTLE_NOEXCEPT
    {
    }
};

static void nested_lock(castle::recursive_mutex& lock, int depth)
{
    lock.lock();

    if (depth > 0)
    {
        nested_lock(lock, depth - 1);
    }

    lock.unlock();
}

int main()
{
    castle::recursive_mutex lock;

    CASTLE_SAMPLE_CHECK(lock.try_lock());
    CASTLE_SAMPLE_CHECK(lock.try_lock());
    lock.unlock();
    lock.unlock();

    nested_lock(lock, 2);

    castle::basic_recursive_mutex<counting_wait_policy> custom_lock;
    custom_lock.lock();
    CASTLE_SAMPLE_CHECK(custom_lock.try_lock());
    custom_lock.unlock();
    custom_lock.unlock();

    return 0;
}
