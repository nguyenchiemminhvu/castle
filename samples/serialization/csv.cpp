#include "sample_support.hpp"

#include "castle/serialization/csv.hpp"

#include <stdint.h>

// Scenario: parse a small telemetry table, read typed fields, construct a row
// programmatically, and serialize it without dynamic allocation.
int main()
{
    using csv_t = castle::serialization::csv::document<8U, 8U, 32U>;

    const castle::container::string_view source(
        "signal,value,enabled\n"
        "rpm,2500,true\n"
        "temperature,87.5,false\n");

    csv_t table;
    const castle::serialization::csv::result parse_result =
        castle::serialization::csv::parse(table, source);
    if (!parse_result.succeeded())
    {
        return 1;
    }

    uint16_t rpm = 0U;
    bool enabled = false;
    if (table.get_integer(1U, 1U, rpm) != castle::status::ok ||
        table.get_bool(1U, 2U, enabled) != castle::status::ok)
    {
        return 2;
    }

    CASTLE_SAMPLE_CHECK(rpm == 2500U && enabled == true);

    castle::size_type row = csv_t::npos;
    if (table.add_row(row) != castle::status::ok ||
        table.append(row, "voltage") != castle::status::ok ||
        table.set_floating(row, 1U, 12.5, 2U) != castle::status::ok ||
        table.set_bool(row, 2U, true) != castle::status::ok)
    {
        return 3;
    }

    castle::container::string<256U> output;
    const castle::serialization::csv::result write_result =
        castle::serialization::csv::write(table, output);
    if (!write_result.succeeded())
    {
        return 4;
    }

    // output.c_str() / output.view() can now be sent to a transport.
    (void)rpm;
    (void)enabled;
    (void)output;
    return 0;
}
