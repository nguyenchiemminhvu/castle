#include "sample_support.hpp"

#include "castle/callbacks/subscription.hpp"

#include <stdint.h>

namespace
{
class mock_owner : public castle::callbacks::i_unsubscribable
{
public:
    castle::status unsubscribe_slot(castle::size_type index, uint32_t generation) noexcept override
    {
        ++calls;

        if (!active || index != expected_index || generation != expected_generation)
        {
            return castle::status::invalid_subscription;
        }

        active = false;
        return castle::status::ok;
    }

    castle::size_type expected_index = 3U;
    uint32_t expected_generation = 7U;
    uint32_t calls = 0U;
    bool active = true;
};
} // namespace

int main()
{
    castle::callbacks::subscription empty{};
    CASTLE_SAMPLE_CHECK(!empty.valid());
    CASTLE_SAMPLE_CHECK(!empty);
    CASTLE_SAMPLE_CHECK(empty.unsubscribe() == castle::status::invalid_subscription);

    mock_owner owner{};
    castle::callbacks::subscription active{&owner, owner.expected_index, owner.expected_generation};
    castle::callbacks::subscription copy = active;

    CASTLE_SAMPLE_CHECK(active.valid());
    CASTLE_SAMPLE_CHECK(active.index() == 3U);
    CASTLE_SAMPLE_CHECK(active.generation() == 7U);

    castle::callbacks::subscription manual_reset{&owner, 1U, 2U};
    manual_reset.reset();
    CASTLE_SAMPLE_CHECK(!manual_reset.valid());
    CASTLE_SAMPLE_CHECK(owner.calls == 0U);

    CASTLE_SAMPLE_CHECK(active.unsubscribe() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(!active.valid());
    CASTLE_SAMPLE_CHECK(owner.calls == 1U);

    CASTLE_SAMPLE_CHECK(copy.unsubscribe() == castle::status::invalid_subscription);
    CASTLE_SAMPLE_CHECK(owner.calls == 2U);

    return 0;
}
