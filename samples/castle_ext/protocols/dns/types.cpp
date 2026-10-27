#include "sample_support.hpp"
#include "castle_ext/protocols/dns/types.hpp"

int main()
{
    using namespace castle::protocols::dns;
    response<4U> answer;
    client_config config;
    config.timeout_ms = 750U;
    config.retries = 1U;
    answer.records[0U].type = record_type::a;
    answer.record_count = 1U;
    CASTLE_SAMPLE_CHECK(answer.record_count == 1U);
    answer.clear();
    CASTLE_SAMPLE_CHECK(answer.record_count == 0U);
    CASTLE_SAMPLE_CHECK(answer.rcode == response_code::no_error);
    CASTLE_SAMPLE_CHECK(config.timeout_ms == 750U && config.retries == 1U);
    return 0;
}
