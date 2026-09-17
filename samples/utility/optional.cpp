#include "sample_support.hpp"

#include "castle/utility/optional.hpp"

#include <stdint.h>

struct Reading
{
    uint32_t id;
    uint32_t value;

    Reading(uint32_t reading_id, uint32_t reading_value) noexcept
        : id(reading_id)
        , value(reading_value)
    {
    }
};

int main()
{
    castle::optional<uint32_t> value;
    CASTLE_SAMPLE_CHECK(!value.has_value());
    CASTLE_SAMPLE_CHECK(value == castle::nullopt);
    CASTLE_SAMPLE_CHECK(value.begin() == nullptr);
    CASTLE_SAMPLE_CHECK(value.end() == nullptr);

    value = 7U;
    CASTLE_SAMPLE_CHECK(value.has_value());
    CASTLE_SAMPLE_CHECK(static_cast<bool>(value));
    CASTLE_SAMPLE_CHECK(value.value() == 7U);
    CASTLE_SAMPLE_CHECK(*value == 7U);
    CASTLE_SAMPLE_CHECK(value.value_or(9U) == 7U);
    CASTLE_SAMPLE_CHECK(value.begin() != nullptr);
    CASTLE_SAMPLE_CHECK(value.end() == value.begin() + 1U);
    CASTLE_SAMPLE_CHECK(value.cbegin() == value.begin());
    CASTLE_SAMPLE_CHECK(value.cend() == value.end());

    castle::optional<uint32_t> copied(value);
    CASTLE_SAMPLE_CHECK(copied == value);
    castle::optional<uint32_t> moved(castle::move(copied));
    CASTLE_SAMPLE_CHECK(moved.value() == 7U);

    value.reset();
    CASTLE_SAMPLE_CHECK(!value);
    CASTLE_SAMPLE_CHECK(value.value_or(11U) == 11U);

    value.emplace(42U);
    CASTLE_SAMPLE_CHECK(value.value() == 42U);

    castle::optional<Reading> reading(castle::meta::in_place, 3U, 99U);
    CASTLE_SAMPLE_CHECK(reading->id == 3U);
    CASTLE_SAMPLE_CHECK((*reading).value == 99U);

    castle::optional<uint32_t> other(5U);
    value.swap(other);
    CASTLE_SAMPLE_CHECK(value.value() == 5U);
    CASTLE_SAMPLE_CHECK(other.value() == 42U);
    castle::swap(value, other);
    CASTLE_SAMPLE_CHECK(value.value() == 42U);
    CASTLE_SAMPLE_CHECK(other.value() == 5U);

    CASTLE_SAMPLE_CHECK(value != other);
    CASTLE_SAMPLE_CHECK(other < value);
    CASTLE_SAMPLE_CHECK(value > castle::nullopt);
    CASTLE_SAMPLE_CHECK(castle::nullopt < value);
    CASTLE_SAMPLE_CHECK(castle::nullopt <= value);
    CASTLE_SAMPLE_CHECK(value >= castle::nullopt);
    CASTLE_SAMPLE_CHECK(castle::make_optional(123U).value() == 123U);
    return 0;
}
