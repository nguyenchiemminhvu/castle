#include "sample_support.hpp"

#include "castle/memory/alignment.hpp"
#include "castle/memory/new.hpp"

#include <stdint.h>

struct Packet
{
    uint32_t value;
};

int main()
{
    castle::memory::aligned_storage<sizeof(Packet), alignof(Packet)>::type storage{};
    Packet* packet_ptr = storage.get_address<Packet>();

    CASTLE_SAMPLE_CHECK(castle::memory::is_aligned(packet_ptr, alignof(Packet)));
    CASTLE_SAMPLE_CHECK(castle::memory::is_aligned<alignof(Packet)>(packet_ptr));
    CASTLE_SAMPLE_CHECK(castle::memory::is_aligned<Packet>(packet_ptr));

    castle::memory::type_with_alignment_t<alignof(Packet)> token_in_memory{};
    castle::type_with_alignment_t<alignof(Packet)> token_in_castle{};
    CASTLE_SAMPLE_CHECK(castle::memory::is_aligned<alignof(Packet)>(&token_in_memory));
    CASTLE_SAMPLE_CHECK(castle::memory::is_aligned<alignof(Packet)>(&token_in_castle));

    castle::memory::aligned_storage_t<sizeof(Packet), alignof(Packet)> storage_alias{};
    castle::aligned_storage_t<sizeof(Packet), alignof(Packet)> top_level_storage{};
    castle::memory::aligned_storage_as_t<sizeof(Packet), Packet> storage_as{};
    castle::aligned_storage_as_t<sizeof(Packet), Packet> top_level_storage_as{};

    Packet* alias_ptr = storage_alias.get_address<Packet>();
    Packet* top_level_ptr = top_level_storage.get_address<Packet>();
    Packet* packet = ::new (storage_as.get_address<Packet>()) Packet{0xA5U};
    Packet* packet_top_level = ::new (top_level_storage_as.get_address<Packet>()) Packet{0x5AU};
    castle::memory::aligned_storage_as_t<sizeof(Packet), Packet> const& const_storage_as = storage_as;
    castle::aligned_storage_as_t<sizeof(Packet), Packet> const& const_top_level_storage_as = top_level_storage_as;

    CASTLE_SAMPLE_CHECK(alias_ptr != nullptr);
    CASTLE_SAMPLE_CHECK(top_level_ptr != nullptr);
    CASTLE_SAMPLE_CHECK(storage_as.get_reference<Packet>().value == 0xA5U);
    CASTLE_SAMPLE_CHECK(const_storage_as.get_reference<Packet>().value == 0xA5U);
    CASTLE_SAMPLE_CHECK(const_top_level_storage_as.get_address<Packet>() == packet_top_level);
    CASTLE_SAMPLE_CHECK(packet->value == 0xA5U);
    CASTLE_SAMPLE_CHECK(packet_top_level->value == 0x5AU);
    return 0;
}
