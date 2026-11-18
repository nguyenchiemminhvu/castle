#include "sample_support.hpp"

#include "castle/core/config.hpp"

int main()
{
    CASTLE_SAMPLE_CHECK(CASTLE_CPP_17);
    CASTLE_SAMPLE_CHECK(CASTLE_HAS_CONSTEXPR_ENDIANNESS == 1);
    CASTLE_SAMPLE_CHECK(castle::native_endian == castle::endian::native);
    CASTLE_SAMPLE_CHECK(castle::is_little_endian || castle::is_big_endian);
    CASTLE_SAMPLE_CHECK(castle::platform_16bit || castle::platform_32bit || castle::platform_64bit);
    CASTLE_SAMPLE_CHECK(castle::inplace_function_storage_words == 8U);
    CASTLE_SAMPLE_CHECK(castle::inplace_storage_reserved == castle::inplace_function_storage_words * sizeof(void*));
    CASTLE_SAMPLE_CHECK(castle::inplace_alignment_default >= alignof(void*));
    return 0;
}
