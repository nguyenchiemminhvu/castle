#include "castle/bit/bit.h"

#include <iostream>

using namespace castle::bit;

void demo_bit_core()
{
    std::cout << "====== Demo: bit core functions ======" << std::endl;

    uint32_t value = 0b00001111; // Initial value: 15

    test(value, 3U) ? std::cout << "Bit 3 is set." : std::cout << "Bit 3 is not set.";
    test<3>(value) ? std::cout << "Bit 3 is set." : std::cout << "Bit 3 is not set.";
    std::cout << std::endl;

    std::cout << "Setting bit 5..." << std::endl;
    value = set(value, 5U);
    test(value, 5U) ? std::cout << "Bit 5 is set." : std::cout << "Bit 5 is not set.";
    std::cout << std::endl;

    std::cout << "Setting bit 6..." << std::endl;
    value = set<6>(value);
    test(value, 6U) ? std::cout << "Bit 6 is set." : std::cout << "Bit 6 is not set.";
    std::cout << std::endl;

    std::cout << "Clearing bit 5..." << std::endl;
    value = clear(value, 5U);
    test(value, 5U) ? std::cout << "Bit 5 is set." : std::cout << "Bit 5 is not set.";
    std::cout << std::endl;

    std::cout << "Clearing bit 6..." << std::endl;
    value = clear<6>(value);
    test(value, 6U) ? std::cout << "Bit 6 is set." : std::cout << "Bit 6 is not set.";
    std::cout << std::endl;

    std::cout << "Toggling bit 4..." << std::endl;
    value = toggle(value, 4U);
    test(value, 4U) ? std::cout << "Bit 4 is set." : std::cout << "Bit 4 is not set.";
    std::cout << std::endl;

    std::cout << "Toggling bit 4 again..." << std::endl;
    value = toggle<4>(value);
    test(value, 4U) ? std::cout << "Bit 4 is set." : std::cout << "Bit 4 is not set.";
    std::cout << std::endl;
}

void demo_bit_count()
{
    std::cout << "====== Demo: bit count functions ======" << std::endl;

    uint32_t value = 0b00001111; // Initial value: 15
    uint32_t ones = count_ones(value);
    std::cout << "Number of set bits: " << ones << std::endl;

    value = 0b11110000; // New value: 240
    ones = count_ones(value);
    std::cout << "Number of set bits: " << ones << std::endl;

    uint32_t leading_zeros = count_leading_zeros(value);
    std::cout << "Number of leading zero bits: " << leading_zeros << std::endl;

    uint32_t trailing_zeros = count_trailing_zeros(value);
    std::cout << "Number of trailing zero bits: " << trailing_zeros << std::endl;

    uint32_t bitwidth = bit_width(value);
    std::cout << "Bit width of the value: " << bitwidth << std::endl;

    uint32_t log2floor = log2_floor(value);
    std::cout << "Floor of log2 of the value: " << log2floor << std::endl;

    uint32_t parity_value = parity(value);
    std::cout << "Parity of the value: " << parity_value << std::endl;

    std::cout << "Parity of the value (using template): " << parity<0b00001111>() << std::endl;
}

void demo_bit_endian()
{
    std::cout << "====== Demo: bit endian functions ======" << std::endl;

    uint32_t value = 0b00001111; // Initial value: 15
    uint32_t swapped = byte_swap(value);
    std::cout << "Byte-swapped value: " << swapped << std::endl;
    value = byte_swap(swapped);
    std::cout << "Value after swapping back: " << value << std::endl;
}

void demo_bit_mask()
{
    std::cout << "====== Demo: bit mask functions ======" << std::endl;

    std::cout << "All bits mask for uint32_t: " << all_bits_mask_v<uint32_t> << std::endl;

    std::cout << "Single bit mask for bit 3 (uint32_t): " << single_bit_mask<uint32_t>(3) << std::endl;

    std::cout << "Single bit mask for bit 3 (template, uint32_t): " << single_bit_mask_v<3, uint32_t> << std::endl;

    std::cout << "Low bits mask for bit 3 (uint32_t): " << low_bits_mask<uint32_t>(3) << std::endl;

    std::cout << "Low bits mask for bit 3 (template, uint32_t): " << low_bits_mask_v<3, uint32_t> << std::endl;

    std::cout << "High bits mask for bit 3 (uint32_t): " << high_bits_mask<uint32_t>(3) << std::endl;

    std::cout << "High bits mask for bit 3 (template, uint32_t): " << high_bits_mask_v<3, uint32_t> << std::endl;

    std::cout << "Range mask for start bit 2 and bit count 3 (uint32_t): " << range_mask<uint32_t>(2, 3) << std::endl;

    std::cout << "Range mask for start bit 2 and bit count 3 (template, uint32_t): " << range_mask_v<2, 3, uint32_t> << std::endl;
}

void demo_bit_math()
{
    std::cout << "====== Demo: bit math functions ======" << std::endl;

    uint32_t value = 0b00001111; // Initial value: 15

    std::cout << "Is the value even: " << std::boolalpha << is_even(value) << std::endl;

    std::cout << "Is the value even (using template): " << is_even_v<0b00001111> << std::endl;

    std::cout << "Is the value odd: " << std::boolalpha << is_odd(value) << std::endl;

    std::cout << "Is the value odd (using template): " << is_odd_v<0b00001111> << std::endl;

    std::cout << "Is the value power of two: " << std::boolalpha << is_power_of_two(value) << std::endl;

    std::cout << "Is the value power of two (using template): " << is_power_of_two_v<0b00001111> << std::endl;

    std::cout << "Sign of the value: " << sign(value) << std::endl;

    std::cout << "Sign of the value (using template): " << sign_v<0b00001111> << std::endl;
}

void demo_bit_reverse()
{
    std::cout << "====== Demo: bit reverse functions ======" << std::endl;

    uint32_t value = 0b00001111; // Initial value: 15

    std::cout << "Reversed bits of the value: " << reverse_bits(value) << std::endl;

    std::cout << "Reversed bytes of the value: " << reverse_bytes(value) << std::endl;
}

void demo_bit_rotate()
{
    std::cout << "====== Demo: bit rotate functions ======" << std::endl;

    uint32_t value = 0b00001111; // Initial value: 15

    uint32_t rotated_left = rotate_left(value, 2);
    uint32_t rotated_right = rotate_right(rotated_left, 2);

    std::cout << "Value rotated left by 2: " << rotated_left << std::endl;

    std::cout << "Value rotated right by 2: " << rotated_right << std::endl;
}

void demo_bit_utils()
{
    std::cout << "====== Demo: bit utils functions ======" << std::endl;

    uint32_t value = 0b00110100; // Initial value: 52

    std::cout << "Extracted lowest set bit of the value: " << extract_lowest_set_bit(value) << std::endl;

    std::cout << "Extracted highest set bit of the value: " << extract_highest_set_bit(value) << std::endl;

    std::cout << "Extracted fields of the value: " << extract_field(value, 2, 4) << std::endl;

    std::cout << "Extracted fields of the value (using template): " << extract_field<2, 4>(0b00110100) << std::endl;
}

int main()
{
    demo_bit_core();
    demo_bit_count();
    demo_bit_endian();
    demo_bit_mask();
    demo_bit_math();
    demo_bit_reverse();
    demo_bit_rotate();
    demo_bit_utils();

    return 0;
}