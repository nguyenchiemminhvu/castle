#include "sample_support.hpp"
#include "castle_ext/protocols/dns/dns.hpp"

int main()
{
    castle::protocols::dns::response<8U> answer;
    CASTLE_SAMPLE_CHECK(answer.record_count == 0U);
    CASTLE_SAMPLE_CHECK(castle::protocols::dns::message::header_size == 12U);
    return 0;
}
