#include "sample_support.hpp"

#include "castle_ext/protocols/ubx/messages/detail/repeated_group.hpp"

namespace
{

namespace ubx = castle::protocols::ubx;
namespace detail = castle::protocols::ubx::messages::detail;

/** @brief Example 4-byte block: satellite ID, C/N0 and a signed residual. */
struct sv_block
{
    static CASTLE_CONSTEXPR castle::size_type wire_size = 4U;

    uint8_t sv_id = 0U;
    uint8_t cno = 0U;
    int16_t residual = 0;

    template <typename Self, typename Stream>
    static void layout(Self& self, Stream& stream) CASTLE_NOEXCEPT
    {
        stream.field(self.sv_id);
        stream.field(self.cno);
        stream.field(self.residual);
    }
};

} // namespace

int main()
{
    using castle::container::array_view;

    // Payload = 1-byte count header + 3 blocks.
    CASTLE_CONST castle::size_type header = 1U;
    CASTLE_SAMPLE_CHECK(detail::has_group_length(13U, header, 3U, sv_block::wire_size));
    CASTLE_SAMPLE_CHECK(!detail::has_group_length(12U, header, 3U, sv_block::wire_size));

    castle::size_type implied = 0U;
    CASTLE_SAMPLE_CHECK(detail::implied_group_count(13U, header, sv_block::wire_size, implied));
    CASTLE_SAMPLE_CHECK(implied == 3U);
    CASTLE_SAMPLE_CHECK(!detail::implied_group_count(12U, header, sv_block::wire_size, implied));

    // Encode three blocks.
    castle::container::vector<sv_block, 3U> blocks;
    for (uint8_t index = 0U; index < 3U; ++index)
    {
        sv_block block{};
        block.sv_id = static_cast<uint8_t>(10U + index);
        block.cno = static_cast<uint8_t>(40U + index);
        block.residual = static_cast<int16_t>(-100 * (index + 1));
        CASTLE_SAMPLE_CHECK(castle::succeeded(blocks.push_back(block)));
    }

    uint8_t payload[13U] = {};
    ubx::payload_writer writer(array_view<uint8_t>(payload, sizeof(payload)));
    writer.write(static_cast<uint8_t>(blocks.size()));
    detail::write_group(writer, blocks);
    CASTLE_SAMPLE_CHECK(writer.ok());
    CASTLE_SAMPLE_CHECK(writer.size() == sizeof(payload));

    // Decode into a 2-slot vector: the third block is skipped, the cursor stays aligned.
    ubx::payload_reader reader(array_view<const uint8_t>(payload, sizeof(payload)));
    CASTLE_CONST uint8_t wire_count = reader.read_u8();
    castle::container::vector<sv_block, 2U> stored;
    detail::read_group(reader, wire_count, stored);
    CASTLE_SAMPLE_CHECK(reader.ok());
    CASTLE_SAMPLE_CHECK(wire_count == 3U);
    CASTLE_SAMPLE_CHECK(stored.size() == 2U);
    CASTLE_SAMPLE_CHECK(stored[1U].sv_id == 11U);
    CASTLE_SAMPLE_CHECK(stored[1U].residual == -200);
    CASTLE_SAMPLE_CHECK(reader.remaining() == 0U);

    // Free-length tails (e.g. MGA-DBD data).
    CASTLE_CONST uint8_t tail_bytes[4U] = {0xDEU, 0xADU, 0xBEU, 0xEFU};
    ubx::payload_reader tail_reader(array_view<const uint8_t>(tail_bytes, sizeof(tail_bytes)));
    castle::container::vector<uint8_t, 8U> tail;
    detail::read_tail(tail_reader, tail);
    CASTLE_SAMPLE_CHECK(tail.size() == 4U);

    uint8_t echo[4U] = {};
    ubx::payload_writer tail_writer(array_view<uint8_t>(echo, sizeof(echo)));
    detail::write_tail(tail_writer, tail);
    CASTLE_SAMPLE_CHECK(tail_writer.ok());
    CASTLE_SAMPLE_CHECK(echo[3U] == 0xEFU);

    return 0;
}
