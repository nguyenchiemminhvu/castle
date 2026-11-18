#include "sample_support.hpp"

#include "castle_ext/statistic/validation.hpp"

int main()
{
    // Float samples keep their precision; integral samples report in double.
    static_assert(castle::meta::is_same<castle::statistic::result_t<float>, float>::value, "float stays float");
    static_assert(castle::meta::is_same<castle::statistic::result_t<uint16_t>, double>::value,
                  "integers report in double");

    // A receiver driver can screen a reported accuracy before using it.
    const double good_accuracy = 2.5;
    const double infinite_accuracy = __builtin_inf();
    CASTLE_SAMPLE_CHECK(castle::statistic::is_finite(good_accuracy));
    CASTLE_SAMPLE_CHECK(!castle::statistic::is_finite(infinite_accuracy));
    CASTLE_SAMPLE_CHECK(!castle::statistic::is_finite(-infinite_accuracy));

    // NaN is the only value that differs from itself.
    const double not_a_number = infinite_accuracy - infinite_accuracy;
    CASTLE_SAMPLE_CHECK(!castle::statistic::is_finite(not_a_number));
    CASTLE_SAMPLE_CHECK(!castle::statistic::is_probability(not_a_number));

    // Integral samples are always finite.
    CASTLE_SAMPLE_CHECK(castle::statistic::is_finite(static_cast<uint16_t>(65535U)));

    // Probabilities (for example a CEP level) must lie in [0, 1].
    CASTLE_SAMPLE_CHECK(castle::statistic::is_probability(0.95));
    CASTLE_SAMPLE_CHECK(castle::statistic::is_probability(0.0));
    CASTLE_SAMPLE_CHECK(castle::statistic::is_probability(1.0));
    CASTLE_SAMPLE_CHECK(!castle::statistic::is_probability(1.5));
    CASTLE_SAMPLE_CHECK(!castle::statistic::is_probability(-0.01));

    return 0;
}
