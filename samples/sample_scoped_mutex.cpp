#include "sample_support.h"

#include "castle/mutex/scoped_mutex.h"

// Scenario: RAII locking of a very short critical section while preserving exception-free code.
int main()
{
    castle::mutex lock;
    {
        castle::scoped_mutex guard(lock);
        (void)guard;
    }
    return 0;
}
