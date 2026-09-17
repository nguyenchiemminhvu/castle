#include "sample_support.hpp"

#include "castle/memory/construct.hpp"
#include "castle/memory/destroy.hpp"
#include "castle/memory/static_storage.hpp"

#include <stdint.h>

struct Tracked
{
    Tracked(uint32_t id_value, uint32_t* log_buffer, uint32_t* size_ptr)
        : id(id_value), log(log_buffer), size(size_ptr) {}

    ~Tracked() noexcept
    {
        log[*size] = id;
        *size += 1U;
    }

    uint32_t id;
    uint32_t* log;
    uint32_t* size;
};

int main()
{
    uint32_t destroy_log[3] = {0U, 0U, 0U};
    uint32_t destroy_count = 0U;
    castle::memory::static_storage<Tracked, 3U> storage;

    Tracked* first = castle::memory::construct_at<Tracked>(storage.address(0U), 1U, destroy_log, &destroy_count);
    Tracked* second = castle::memory::construct_at<Tracked>(storage.address(1U), 2U, destroy_log, &destroy_count);
    Tracked* third = castle::memory::construct_at<Tracked>(storage.address(2U), 3U, destroy_log, &destroy_count);

    CASTLE_SAMPLE_CHECK(second != nullptr);
    castle::memory::destroy_at(third);
    castle::memory::destroy_n(first, 2U);

    CASTLE_SAMPLE_CHECK(destroy_count == 3U);
    CASTLE_SAMPLE_CHECK(destroy_log[0] == 3U);
    CASTLE_SAMPLE_CHECK(destroy_log[1] == 2U);
    CASTLE_SAMPLE_CHECK(destroy_log[2] == 1U);
    return 0;
}
