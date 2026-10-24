#include "sample_support.hpp"
#include "castle_ext/comm/http/http2.hpp"

using namespace castle::comm::http;

int main()
{
    http2_session<2U, 512U, 16384U, 128U, 8U, 32U, 96U, 0U> session;
    CASTLE_SAMPLE_CHECK(session.start() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(session.pending_output_size() >= 24U);
    const uint8_t expected_preface[] = {'P','R','I',' ','*',' ','H','T','T','P','/','2','.','0','\r','\n','\r','\n','S','M','\r','\n','\r','\n'};
    for (castle::size_type i = 0U; i < sizeof(expected_preface); ++i)
    {
        CASTLE_SAMPLE_CHECK(session.pending_output()[i] == expected_preface[i]);
    }
    CASTLE_SAMPLE_CHECK(session.consume_output(session.output_size()) == castle::status::ok);
    http2_session<2U, 512U, 16384U, 128U, 8U, 32U, 96U, 0U>::headers_type headers;
    uint32_t stream_id = 0U;
    CASTLE_SAMPLE_CHECK(session.open_stream(method::get, "/", "node.local:80", headers,
                                            string_view(), stream_id) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(stream_id == 1U && session.active_stream_count() == 1U);
    return 0;
}
