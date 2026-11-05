#include "sample_support.hpp"

#include "castle/utility/any.hpp"

#include <stdint.h>

struct Calibration
{
    uint16_t offset;
    uint16_t gain;

    Calibration(uint16_t calibration_offset, uint16_t calibration_gain) noexcept
        : offset(calibration_offset)
        , gain(calibration_gain)
    {
    }
};

struct Oversized
{
    uint8_t bytes[castle::any::capacity + 1U];
};

int main()
{
    // Type identity without RTTI, shared with castle::type_info.
    CASTLE_SAMPLE_CHECK(castle::any(uint32_t(1U)).type() == castle::type_id<const uint32_t&>());
    CASTLE_SAMPLE_CHECK(castle::any(uint32_t(1U)).type() != castle::type_id<int32_t>());

    // Compile-time storage rules.
    static_assert(castle::any::is_supported_type<Calibration>(), "Calibration fits castle::any");
    static_assert(!castle::any::is_supported_type<Oversized>(), "Oversized is rejected at compile time");
    static_assert(castle::basic_any<8U, 4U>::capacity == 8U, "capacity is exposed");
    static_assert(castle::basic_any<8U, 4U>::alignment == 4U, "alignment is exposed");

    // Empty state.
    castle::any parameter;
    CASTLE_SAMPLE_CHECK(!parameter.has_value());
    CASTLE_SAMPLE_CHECK(parameter.type() == castle::type_id<void>());
    CASTLE_SAMPLE_CHECK(castle::any_cast<uint32_t>(&parameter) == nullptr);

    // Store a value and access it through the checked pointer form.
    parameter = 42U;
    CASTLE_SAMPLE_CHECK(parameter.has_value());
    CASTLE_SAMPLE_CHECK(parameter.type() == castle::type_id<unsigned int>());
    if (unsigned int* value = castle::any_cast<unsigned int>(&parameter))
    {
        *value += 1U;
    }
    CASTLE_SAMPLE_CHECK(castle::any_cast<unsigned int>(parameter) == 43U);
    CASTLE_SAMPLE_CHECK(castle::any_cast<float>(&parameter) == nullptr);

    // Reference access and const access.
    castle::any_cast<unsigned int&>(parameter) = 7U;
    CASTLE_SAMPLE_CHECK(castle::any_cast<const unsigned int&>(parameter) == 7U);
    CASTLE_SAMPLE_CHECK(*castle::any_cast<unsigned int>(&static_cast<const castle::any&>(parameter)) == 7U);

    // Replace the stored type.
    parameter = 2.5F;
    CASTLE_SAMPLE_CHECK(castle::any_cast<float>(parameter) == 2.5F);

    // In-place construction and emplace.
    castle::any calibration(castle::meta::in_place_type<Calibration>, uint16_t(10U), uint16_t(3U));
    CASTLE_SAMPLE_CHECK(castle::any_cast<Calibration&>(calibration).gain == 3U);
    Calibration& updated = calibration.emplace<Calibration>(uint16_t(20U), uint16_t(4U));
    CASTLE_SAMPLE_CHECK(updated.offset == 20U);

    // Copy keeps both values; move relocates and empties the source.
    castle::any copy(calibration);
    CASTLE_SAMPLE_CHECK(castle::any_cast<Calibration&>(copy).offset == 20U);
    castle::any moved(castle::move(copy));
    CASTLE_SAMPLE_CHECK(!copy.has_value());
    CASTLE_SAMPLE_CHECK(castle::any_cast<Calibration>(castle::move(moved)).gain == 4U);

    copy = calibration;
    moved = castle::move(copy);
    CASTLE_SAMPLE_CHECK(moved.type() == castle::type_id<Calibration>());

    // Swap.
    castle::any number(5);
    number.swap(parameter);
    CASTLE_SAMPLE_CHECK(castle::any_cast<float>(number) == 2.5F);
    CASTLE_SAMPLE_CHECK(castle::any_cast<int>(parameter) == 5);
    castle::swap(number, parameter);
    CASTLE_SAMPLE_CHECK(castle::any_cast<int>(number) == 5);

    // Reset.
    number.reset();
    CASTLE_SAMPLE_CHECK(!number.has_value());

    // Custom capacity via make_any.
    castle::basic_any<8U, 4U> small = castle::make_any<uint16_t, 8U, 4U>(uint16_t(9U));
    CASTLE_SAMPLE_CHECK(castle::any_cast<uint16_t>(small) == 9U);
    castle::any standard = castle::make_any<Calibration>(uint16_t(1U), uint16_t(2U));
    CASTLE_SAMPLE_CHECK(castle::any_cast<const Calibration&>(standard).gain == 2U);
    return 0;
}
