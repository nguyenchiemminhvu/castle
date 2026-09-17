#include "sample_support.hpp"

#include "castle/utility/bitset.hpp"

#include <stdint.h>

int main()
{
    castle::bitset<8U> flags(0x12U);
    CASTLE_SAMPLE_CHECK(castle::bitset<8U>::size() == 8U);
    CASTLE_SAMPLE_CHECK(castle::bitset<8U>::number_of_words() >= 1U);
    CASTLE_SAMPLE_CHECK(castle::bitset<8U>::bits_per_word() >= 8U);
    CASTLE_SAMPLE_CHECK(flags.to_ulong() == 0x12U);
    CASTLE_SAMPLE_CHECK(flags.to_ullong() == 0x12ULL);
    CASTLE_SAMPLE_CHECK(flags.count() == 2U);
    CASTLE_SAMPLE_CHECK(flags.any());
    CASTLE_SAMPLE_CHECK(!flags.none());
    CASTLE_SAMPLE_CHECK(!flags.all());
    CASTLE_SAMPLE_CHECK(flags.test(1U));
    CASTLE_SAMPLE_CHECK(!flags.test(0U));
    CASTLE_SAMPLE_CHECK(flags.test(99U) == false);
    CASTLE_SAMPLE_CHECK(flags.find_first(true) == 1U);
    CASTLE_SAMPLE_CHECK(flags.find_next(1U, true) == 4U);
    CASTLE_SAMPLE_CHECK(flags.find_first(false) == 0U);
    CASTLE_SAMPLE_CHECK(flags.find_next(4U, false) == 5U);
    CASTLE_SAMPLE_CHECK(flags.find_next(castle::bitset<8U>::npos(), true) == castle::bitset<8U>::npos());

    flags[0U] = true;
    CASTLE_SAMPLE_CHECK(static_cast<bool>(flags[0U]));
    CASTLE_SAMPLE_CHECK((~flags[0U]) == false);
    flags[0U].flip();
    CASTLE_SAMPLE_CHECK(flags[0U] == false);

    flags.set(7U).reset(1U).flip(4U);
    CASTLE_SAMPLE_CHECK(flags.test(7U));
    CASTLE_SAMPLE_CHECK(!flags.test(1U));
    CASTLE_SAMPLE_CHECK(!flags.test(4U));

    flags.set();
    CASTLE_SAMPLE_CHECK(flags.all());
    flags.reset();
    CASTLE_SAMPLE_CHECK(flags.none());

    flags.assign(0x0FU);
    castle::bitset<8U> parsed("10100101");
    char buffer[9] = {};
    CASTLE_SAMPLE_CHECK(parsed.to_string(buffer, sizeof(buffer)));
    CASTLE_SAMPLE_CHECK(buffer[0] == '1' && buffer[7] == '1' && buffer[8] == '\0');
    CASTLE_SAMPLE_CHECK(parsed.to_ulong() == 0xA5U);

    uint32_t* words = flags.data();
    const castle::bitset<8U>& const_flags = flags;
    CASTLE_SAMPLE_CHECK(words != nullptr);
    CASTLE_SAMPLE_CHECK(const_flags.data() != nullptr);

    castle::bitset<8U> lhs(0x33U);
    castle::bitset<8U> rhs(0x0FU);
    CASTLE_SAMPLE_CHECK((lhs & rhs).to_ulong() == 0x03U);
    CASTLE_SAMPLE_CHECK((lhs | rhs).to_ulong() == 0x3FU);
    CASTLE_SAMPLE_CHECK((lhs ^ rhs).to_ulong() == 0x3CU);
    CASTLE_SAMPLE_CHECK((lhs << 2U).to_ulong() == 0xCCU);
    CASTLE_SAMPLE_CHECK((lhs >> 1U).to_ulong() == 0x19U);
    CASTLE_SAMPLE_CHECK((~rhs).to_ulong() == 0xF0U);

    lhs &= rhs;
    CASTLE_SAMPLE_CHECK(lhs.to_ulong() == 0x03U);
    lhs = castle::bitset<8U>(0x33U);
    lhs |= rhs;
    CASTLE_SAMPLE_CHECK(lhs.to_ulong() == 0x3FU);
    lhs ^= rhs;
    CASTLE_SAMPLE_CHECK(lhs.to_ulong() == 0x30U);
    lhs <<= 1U;
    CASTLE_SAMPLE_CHECK(lhs.to_ulong() == 0x60U);
    lhs >>= 2U;
    CASTLE_SAMPLE_CHECK(lhs.to_ulong() == 0x18U);

    castle::bitset<8U> swapped_a(0xAAU);
    castle::bitset<8U> swapped_b(0x55U);
    swapped_a.swap(swapped_b);
    CASTLE_SAMPLE_CHECK(swapped_a.to_ulong() == 0x55U);
    castle::swap(swapped_a, swapped_b);
    CASTLE_SAMPLE_CHECK(swapped_a == castle::bitset<8U>(0xAAU));
    CASTLE_SAMPLE_CHECK(swapped_a != swapped_b);

    return 0;
}
