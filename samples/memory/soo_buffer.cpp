#include "sample_support.hpp"

#include "castle/memory/soo_buffer.hpp"

#include <stdint.h>

struct Packet
{
    explicit Packet(uint32_t initial) : value(initial) {}

    Packet(Packet&& other) noexcept : value(other.value)
    {
        other.value = 0U;
    }

    Packet(Packet const&) = delete;
    Packet& operator=(Packet const&) = delete;

    uint32_t value;
};

int main()
{
    castle::memory::soo_buffer<Packet> buffer(Packet(7U));
    Packet* mutable_view = buffer.get();
    Packet const* const_view = static_cast<castle::memory::soo_buffer<Packet> const&>(buffer).get();

    CASTLE_SAMPLE_CHECK(mutable_view != nullptr);
    CASTLE_SAMPLE_CHECK(const_view != nullptr);
    CASTLE_SAMPLE_CHECK(mutable_view == const_view);
    CASTLE_SAMPLE_CHECK(mutable_view->value == 7U);
    return 0;
}
