#ifndef CASTLE_EXT_TEST_SUPPORT_HPP
#define CASTLE_EXT_TEST_SUPPORT_HPP

#include <gtest/gtest.h>
#include <stddef.h>
#include <stdint.h>

inline size_t hex_decode_test(const char* text, uint8_t* output, size_t capacity)
{
    size_t length = 0U;
    while (text[length] != '\0') ++length;
    if ((length & 1U) != 0U || length / 2U > capacity) return 0U;
    for (size_t i = 0U; i < length / 2U; ++i)
    {
        auto nibble = [](char c) -> uint8_t {
            if (c >= '0' && c <= '9') return static_cast<uint8_t>(c - '0');
            if (c >= 'a' && c <= 'f') return static_cast<uint8_t>(c - 'a' + 10);
            if (c >= 'A' && c <= 'F') return static_cast<uint8_t>(c - 'A' + 10);
            return 0xFFU;
        };
        const uint8_t high = nibble(text[2U * i]);
        const uint8_t low = nibble(text[2U * i + 1U]);
        if (high == 0xFFU || low == 0xFFU) return 0U;
        output[i] = static_cast<uint8_t>((high << 4U) | low);
    }
    return length / 2U;
}


inline void expect_text(const char* actual,
                        size_t actual_size,
                        const char* expected,
                        size_t expected_size)
{
    ASSERT_EQ(actual_size, expected_size);
    for (size_t i = 0U; i < expected_size; ++i)
    {
        EXPECT_EQ(actual[i], expected[i]) << "character index " << i;
    }
}

inline void expect_bytes(const uint8_t* actual,
                         size_t actual_size,
                         const uint8_t* expected,
                         size_t expected_size)
{
    ASSERT_EQ(actual_size, expected_size);
    for (size_t i = 0U; i < expected_size; ++i)
    {
        EXPECT_EQ(actual[i], expected[i]) << "byte index " << i;
    }
}

#endif // CASTLE_EXT_TEST_SUPPORT_HPP
