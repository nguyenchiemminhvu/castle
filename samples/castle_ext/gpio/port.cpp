#include "sample_support.hpp"

#include "castle_ext/gpio/gpio.hpp"
#include "castle_ext/gpio/backends/mock.hpp"

using namespace castle::gpio;

int main()
{
    // basic_port<Backend> adds no storage beyond the backend's own native handle.
    static_assert(sizeof(port<mock::port_backend>) == sizeof(mock::port_backend::native_handle_type),
                  "basic_port must not add hidden storage");

    mock::port_state bank_state;
    port<mock::port_backend> bank(&bank_state);

    CASTLE_SAMPLE_CHECK(castle::succeeded(bank.write(0x00FFU)));
    mask_type value = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(bank.read(value)));
    CASTLE_SAMPLE_CHECK(value == 0x00FFU);

    // set()/clear() touch only the bits selected by their mask.
    CASTLE_SAMPLE_CHECK(castle::succeeded(bank.set(0x0100U)));
    CASTLE_SAMPLE_CHECK(castle::succeeded(bank.clear(0x000FU)));
    CASTLE_SAMPLE_CHECK(castle::succeeded(bank.read(value)));
    CASTLE_SAMPLE_CHECK(value == 0x01F0U);

    // write_masked() replaces only the selected bits with the matching value bits.
    CASTLE_SAMPLE_CHECK(castle::succeeded(bank.write_masked(0x00FFU, 0x000AU)));
    CASTLE_SAMPLE_CHECK(castle::succeeded(bank.read(value)));
    CASTLE_SAMPLE_CHECK(value == 0x010AU);

    CASTLE_SAMPLE_CHECK(castle::succeeded(bank.toggle(0x0003U)));
    CASTLE_SAMPLE_CHECK(castle::succeeded(bank.read(value)));
    CASTLE_SAMPLE_CHECK(value == 0x0109U);

    // An active-low bank: read()/write() flip the bits selected by active_low_mask,
    // read_raw() always reports the physical register contents.
    mock::port_state inverted_bank_state(/* initial */ 0U, /* invert_mask */ 0x000FU);
    port<mock::port_backend> inverted_bank(&inverted_bank_state);

    CASTLE_SAMPLE_CHECK(castle::succeeded(inverted_bank.write(0x000FU)));
    mask_type physical = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(inverted_bank.read_raw(physical)));
    CASTLE_SAMPLE_CHECK(physical == 0x0000U); // inverted on the wire

    mask_type logical = 0U;
    CASTLE_SAMPLE_CHECK(castle::succeeded(inverted_bank.read(logical)));
    CASTLE_SAMPLE_CHECK(logical == 0x000FU);

    return 0;
}

