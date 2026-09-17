#include "sample_support.hpp"

#include "castle/sync/scoped_mutex.hpp"

struct counting_wait_policy
{
    static void wait() CASTLE_NOEXCEPT
    {
    }
};

int main()
{
    castle::mutex lock;

    {
        castle::scoped_mutex guard(lock);
        (void)guard;
        CASTLE_SAMPLE_CHECK(!lock.try_lock());
    }

    CASTLE_SAMPLE_CHECK(lock.try_lock());
    lock.unlock();

    castle::basic_mutex<counting_wait_policy> custom_lock;

    {
        castle::basic_scoped_mutex<counting_wait_policy> guard(custom_lock);
        (void)guard;
        CASTLE_SAMPLE_CHECK(!custom_lock.try_lock());
    }

    CASTLE_SAMPLE_CHECK(custom_lock.try_lock());
    custom_lock.unlock();

    return 0;
}
