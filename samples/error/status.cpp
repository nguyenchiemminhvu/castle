#include "sample_support.hpp"

#include "castle/error/status.hpp"

int main()
{
    CASTLE_SAMPLE_CHECK(castle::succeeded(castle::status::ok));
    CASTLE_SAMPLE_CHECK(!castle::succeeded(castle::status::full));
    CASTLE_SAMPLE_CHECK(castle::status::not_found != castle::status::already_exists);
    CASTLE_SAMPLE_CHECK(castle::status::invalid_argument != castle::status::out_of_range);
    return 0;
}
