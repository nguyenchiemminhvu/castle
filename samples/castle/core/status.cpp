#include "sample_support.hpp"

#include "castle/core/status.hpp"

int main()
{
    CASTLE_SAMPLE_CHECK(castle::succeeded(castle::status::ok));
    CASTLE_SAMPLE_CHECK(!castle::succeeded(castle::status::full));
    CASTLE_SAMPLE_CHECK(!castle::succeeded(castle::status::in_progress));
    CASTLE_SAMPLE_CHECK(castle::status::not_found != castle::status::already_exists);
    CASTLE_SAMPLE_CHECK(castle::status::invalid_argument != castle::status::parse_error);
    CASTLE_SAMPLE_CHECK(castle::status::not_converged != castle::status::insufficient_data);
    return 0;
}
