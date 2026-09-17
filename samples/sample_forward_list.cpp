/**
 * Castle 2.0 sample: <castle/container/forward_list.h>
 *
 * Scenario: a bounded command queue where nodes stay in fixed internal storage.
 */
#include "sample_support.h"
#include "castle/container/forward_list.h"

#include <stdint.h>

int main()
{
    castle::container::forward_list<uint32_t, 4U> queue;

    CASTLE_SAMPLE_CHECK(queue.push_front(10U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(queue.push_front(20U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(queue.front() == 20U);
    CASTLE_SAMPLE_CHECK(queue.size() == 2U);

    queue.pop_front();
    CASTLE_SAMPLE_CHECK(queue.front() == 10U);
    queue.clear();
    CASTLE_SAMPLE_CHECK(queue.empty());
    return 0;
}
