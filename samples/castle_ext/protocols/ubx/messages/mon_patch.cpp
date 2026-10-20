#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/mon_patch.hpp"

int main()
{
    using namespace castle::protocols::ubx;
    using castle::protocols::ubx::messages::mon_patch;
    using castle::protocols::ubx::messages::mon_patch_entry;
    using castle::protocols::ubx::messages::mon_patch_location;

    // Two installed patches: an active ROM patch and an inactive BBR patch.
    mon_patch<4U> patches{};
    patches.version = 1U;
    mon_patch_entry rom_patch{};
    rom_patch.patch_info = mon_patch_entry::PATCH_INFO_ACTIVATED | (1U << mon_patch_entry::PATCH_INFO_LOCATION_SHIFT);
    rom_patch.comparator = 3U;
    rom_patch.patch_address = 0x00201000U;
    rom_patch.patch_data = 0xDEADBEEFU;
    CASTLE_SAMPLE_CHECK(patches.entries.push_back(rom_patch) == castle::status::ok);
    mon_patch_entry bbr_patch{};
    bbr_patch.patch_info = 2U << mon_patch_entry::PATCH_INFO_LOCATION_SHIFT;
    CASTLE_SAMPLE_CHECK(patches.entries.push_back(bbr_patch) == castle::status::ok);

    uint8_t frame[64U];
    castle::size_type written = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(patches, frame, sizeof(frame), written) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(written == frame_size(mon_patch<4U>::header_length + (2U * mon_patch<4U>::block_length)));

    message_view raw{};
    CASTLE_SAMPLE_CHECK(decode_frame(castle::container::array_view<const uint8_t>(frame, written), raw));
    CASTLE_SAMPLE_CHECK(mon_patch<4U>::matches(raw));

    mon_patch<4U> decoded{};
    CASTLE_SAMPLE_CHECK(mon_patch<4U>::decode(raw, decoded));
    CASTLE_SAMPLE_CHECK(decoded.version == 1U);
    CASTLE_SAMPLE_CHECK(decoded.n_entries == 2U); // taken from entries.size() on encode
    CASTLE_SAMPLE_CHECK(decoded.entries.size() == 2U);
    CASTLE_SAMPLE_CHECK(decoded.entries[0U].activated());
    CASTLE_SAMPLE_CHECK(decoded.entries[0U].location() == mon_patch_location::rom);
    CASTLE_SAMPLE_CHECK(decoded.entries[0U].comparator == 3U);
    CASTLE_SAMPLE_CHECK(decoded.entries[0U].patch_address == 0x00201000U);
    CASTLE_SAMPLE_CHECK(decoded.entries[0U].patch_data == 0xDEADBEEFU);
    CASTLE_SAMPLE_CHECK(!decoded.entries[1U].activated());
    CASTLE_SAMPLE_CHECK(decoded.entries[1U].location() == mon_patch_location::bbr);

    // Storage smaller than the patch list: n_entries keeps the on-wire count.
    mon_patch<1U> first_only{};
    CASTLE_SAMPLE_CHECK(mon_patch<1U>::decode(raw, first_only));
    CASTLE_SAMPLE_CHECK(first_only.n_entries == 2U);
    CASTLE_SAMPLE_CHECK(first_only.entries.size() == 1U);

    // Request the message with an empty poll frame.
    CASTLE_SAMPLE_CHECK(encode_poll<mon_patch<4U>>(frame, sizeof(frame), written) == castle::status::ok);
    return 0;
}
