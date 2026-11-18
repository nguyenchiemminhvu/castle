#include "sample_support.hpp"

#include "castle/utility/variant.hpp"

#include <stdint.h>

struct SensorReading
{
    uint32_t id;
    uint32_t value;

    SensorReading(uint32_t reading_id, uint32_t reading_value) noexcept
        : id(reading_id)
        , value(reading_value)
    {
    }
};

struct reading_visitor
{
    uint32_t operator()(uint32_t value) const noexcept
    {
        return value;
    }

    uint32_t operator()(const SensorReading& value) const noexcept
    {
        return value.value;
    }
};

using message_variant = castle::variant<uint32_t, SensorReading>;
static_assert(message_variant::size == 2U, "variant size constant");
static_assert(castle::variant_size<message_variant>::value == 2U, "variant_size");
static_assert(castle::variant_size_v<message_variant> == 2U, "variant_size_v");
static_assert(castle::meta::is_same<castle::variant_alternative_t<1U, message_variant>, SensorReading>::value, "variant_alternative_t");

int main()
{
    message_variant message(castle::in_place_index<0>, 77U);
    CASTLE_SAMPLE_CHECK(message.index() == 0U);
    CASTLE_SAMPLE_CHECK(message_variant::is_supported_type<uint32_t>());
    CASTLE_SAMPLE_CHECK(castle::holds_alternative<uint32_t>(message));
    CASTLE_SAMPLE_CHECK(castle::holds_alternative<0U>(message));
    CASTLE_SAMPLE_CHECK(castle::get<uint32_t>(message) == 77U);
    CASTLE_SAMPLE_CHECK(castle::get<0U>(message) == 77U);
    CASTLE_SAMPLE_CHECK(castle::get_if<uint32_t>(&message) != nullptr);
    CASTLE_SAMPLE_CHECK(castle::get_if<1U>(&message) == nullptr);
    CASTLE_SAMPLE_CHECK(castle::visit(reading_visitor(), message) == 77U);

    message.emplace<1U>(3U, 9U);
    CASTLE_SAMPLE_CHECK(message.index() == 1U);
    CASTLE_SAMPLE_CHECK(castle::get<1U>(message).value == 9U);
    CASTLE_SAMPLE_CHECK(castle::get<SensorReading>(message).id == 3U);
    CASTLE_SAMPLE_CHECK(castle::visit(reading_visitor(), message) == 9U);

    message_variant other(castle::meta::in_place_type<SensorReading>, 5U, 11U);
    message.swap(other);
    CASTLE_SAMPLE_CHECK(castle::get<SensorReading>(message).id == 5U);
    castle::swap(message, other);
    CASTLE_SAMPLE_CHECK(castle::get<SensorReading>(message).id == 3U);

    message.reset();
    CASTLE_SAMPLE_CHECK(message.valueless_by_exception());
    CASTLE_SAMPLE_CHECK(message.index() == castle::variant_npos);

    auto single = castle::make_variant<uint16_t>(12U);
    CASTLE_SAMPLE_CHECK(castle::get<uint16_t>(single) == 12U);
    return 0;
}
