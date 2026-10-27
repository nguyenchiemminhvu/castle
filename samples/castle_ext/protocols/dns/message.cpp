#include "sample_support.hpp"
#include "castle_ext/protocols/dns/message.hpp"

int main()
{
    using namespace castle;
    using namespace castle::protocols::dns;

    uint8_t query[512U]{};
    size_type query_size = 0U;
    CASTLE_SAMPLE_CHECK(message::encode_query("www.example.com", record_type::a, 0x1234U,
                                              query, sizeof(query), query_size) == status::ok);
    CASTLE_SAMPLE_CHECK(query_size == 33U);
    CASTLE_SAMPLE_CHECK(query[0U] == 0x12U && query[1U] == 0x34U);
    CASTLE_SAMPLE_CHECK(query[2U] == 0x01U && query[5U] == 0x01U);

    uint8_t reply[64U]{};
    for (size_type i = 0U; i < query_size; ++i)
    {
        reply[i] = query[i];
    }
    reply[2U] = 0x81U; reply[3U] = 0x80U;
    reply[6U] = 0U; reply[7U] = 1U;
    size_type at = query_size;
    reply[at++] = 0xC0U; reply[at++] = 0x0CU; // compressed owner name
    reply[at++] = 0U; reply[at++] = 1U;      // A
    reply[at++] = 0U; reply[at++] = 1U;      // IN
    reply[at++] = 0U; reply[at++] = 0U; reply[at++] = 0U; reply[at++] = 60U;
    reply[at++] = 0U; reply[at++] = 4U;
    reply[at++] = 93U; reply[at++] = 184U; reply[at++] = 216U; reply[at++] = 34U;

    response<4U> parsed;
    CASTLE_SAMPLE_CHECK(message::parse_response(reply, at, 0x1234U, parsed) == status::ok);
    CASTLE_SAMPLE_CHECK(parsed.answer_count == 1U && parsed.record_count == 1U);
    CASTLE_SAMPLE_CHECK(parsed.records[0U].address.address_bytes()[0U] == 93U);
    CASTLE_SAMPLE_CHECK(parsed.records[0U].address.address_bytes()[3U] == 34U);
    CASTLE_SAMPLE_CHECK(!message::is_truncated_packet(reply, at));
    return 0;
}
