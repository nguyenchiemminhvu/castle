#include "sample_support.hpp"

#include "castle/core/constants.hpp"

namespace
{

CASTLE_CONSTEXPR double absolute_value(double value) CASTLE_NOEXCEPT
{
    return value < 0.0 ? -value : value;
}

CASTLE_CONSTEXPR bool near_equal(double left, double right, double tolerance) CASTLE_NOEXCEPT
{
    return absolute_value(left - right) <= tolerance;
}

} // namespace

int main()
{
    CASTLE_SAMPLE_CHECK(near_equal(castle::math::pi<double>(), 3.14159265358979323846, 1e-15));
    CASTLE_SAMPLE_CHECK(near_equal(castle::math::tau<double>(), 2.0 * castle::math::pi<double>(), 1e-15));
    CASTLE_SAMPLE_CHECK(near_equal(castle::math::degrees_to_radians(180.0), castle::math::pi<double>(), 1e-15));
    CASTLE_SAMPLE_CHECK(near_equal(castle::math::radians_to_degrees(castle::math::pi<double>()), 180.0, 1e-12));
    CASTLE_SAMPLE_CHECK(castle::math::e<float>() > 2.7f);
    CASTLE_SAMPLE_CHECK(castle::math::golden_ratio<float>() > 1.6f);
    CASTLE_SAMPLE_CHECK(castle::math::log2_e<float>() > 1.4f);
    CASTLE_SAMPLE_CHECK(castle::math::log10_e<float>() > 0.43f);
    CASTLE_SAMPLE_CHECK(castle::math::ln_2<float>() > 0.69f);
    CASTLE_SAMPLE_CHECK(castle::math::ln_10<float>() > 2.30f);
    CASTLE_SAMPLE_CHECK(castle::math::sqrt_2<float>() > 1.41f);
    CASTLE_SAMPLE_CHECK(castle::math::sqrt_3<float>() > 1.73f);
    CASTLE_SAMPLE_CHECK(castle::math::inv_sqrt_2<float>() > 0.70f);

    CASTLE_SAMPLE_CHECK(castle::characters::ascii_upper_min == 0x41U);
    CASTLE_SAMPLE_CHECK(castle::characters::ascii_lower_max == 0x7AU);
    CASTLE_SAMPLE_CHECK(castle::characters::utf8_ascii_max == 0x7FU);
    CASTLE_SAMPLE_CHECK(castle::characters::utf8_four_byte_prefix == 0xF0U);
    CASTLE_SAMPLE_CHECK(castle::characters::utf16_surrogate_min == 0xD800U);

    CASTLE_SAMPLE_CHECK(castle::serialization::max_integer_str_len == 32U);
    CASTLE_SAMPLE_CHECK(castle::serialization::max_floating_str_len == 48U);
    CASTLE_SAMPLE_CHECK(castle::serialization::unicode_max_codepoint == 0x10FFFFU);
    CASTLE_SAMPLE_CHECK(castle::serialization::max_floating_exponent == 1024);
    CASTLE_SAMPLE_CHECK(castle::serialization::xml_tab == 0x09U);
    CASTLE_SAMPLE_CHECK(castle::serialization::xml_supplementary_max == 0x10FFFFU);
    return 0;
}
