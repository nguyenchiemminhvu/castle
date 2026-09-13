#include "sample_support.hpp"

#include "castle/mutex/mutex.hpp"

// Scenario: protecting a short critical section in a no-OS cooperative application.
int main()
{
    castle::mutex lock;
    CASTLE_SAMPLE_CHECK(lock.try_lock());
    lock.unlock();
    lock.lock();
    lock.unlock();
    return 0;
}
