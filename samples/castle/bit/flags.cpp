#include "sample_support.hpp"

#include "castle/bit/flags.hpp"

#include <stdint.h>

int main()
{
    using mode_flags = castle::bit::flags<uint8_t, 0x0FU>;

    CASTLE_SAMPLE_CHECK(mode_flags::ALL_SET == static_cast<uint8_t>(0x0FU));
    CASTLE_SAMPLE_CHECK(mode_flags::ALL_CLEAR == static_cast<uint8_t>(0x00U));
    CASTLE_SAMPLE_CHECK(mode_flags::NBITS == 8U);

    mode_flags flags_default;
    CASTLE_SAMPLE_CHECK(flags_default.none());

    mode_flags flags_value(0xF3U);
    CASTLE_SAMPLE_CHECK(flags_value.value() == static_cast<uint8_t>(0x03U));

    mode_flags flags_copy(flags_value);
    CASTLE_SAMPLE_CHECK(flags_copy == flags_value);

    mode_flags flags_move(castle::move(flags_copy));
    CASTLE_SAMPLE_CHECK(flags_move.value() == static_cast<uint8_t>(0x03U));

    CASTLE_SAMPLE_CHECK(flags_move.test<0x01U>());
    CASTLE_SAMPLE_CHECK(flags_move.test(static_cast<uint8_t>(0x02U)));
    CASTLE_SAMPLE_CHECK(!flags_move.test<0x08U>());

    flags_move.set<0x04U, true>();
    CASTLE_SAMPLE_CHECK(flags_move.value() == static_cast<uint8_t>(0x07U));
    flags_move.set<0x08U>(true);
    CASTLE_SAMPLE_CHECK(flags_move.value() == static_cast<uint8_t>(0x0FU));
    flags_move.set<0x08U>(false);
    CASTLE_SAMPLE_CHECK(flags_move.value() == static_cast<uint8_t>(0x07U));
    flags_move.set<0x08U>();
    CASTLE_SAMPLE_CHECK(flags_move.value() == static_cast<uint8_t>(0x0FU));
    flags_move.set(static_cast<uint8_t>(0x10U));
    CASTLE_SAMPLE_CHECK(flags_move.value() == static_cast<uint8_t>(0x0FU));
    flags_move.set(static_cast<uint8_t>(0x02U), false);
    CASTLE_SAMPLE_CHECK(flags_move.value() == static_cast<uint8_t>(0x0DU));
    flags_move.set(static_cast<uint8_t>(0x02U), true);
    CASTLE_SAMPLE_CHECK(flags_move.value() == static_cast<uint8_t>(0x0FU));

    CASTLE_SAMPLE_CHECK(flags_move.all());
    CASTLE_SAMPLE_CHECK(flags_move.all_of<0x03U>());
    CASTLE_SAMPLE_CHECK(flags_move.all_of(static_cast<uint8_t>(0x0CU)));
    CASTLE_SAMPLE_CHECK(!flags_move.none());
    CASTLE_SAMPLE_CHECK(!flags_move.none_of<0x01U>());
    CASTLE_SAMPLE_CHECK(flags_move.none_of(static_cast<uint8_t>(0x10U)));
    CASTLE_SAMPLE_CHECK(flags_move.any());
    CASTLE_SAMPLE_CHECK(flags_move.any_of<0x08U>());
    CASTLE_SAMPLE_CHECK(!flags_move.any_of(static_cast<uint8_t>(0x10U)));

    flags_move.reset<0x08U>();
    CASTLE_SAMPLE_CHECK(flags_move.value() == static_cast<uint8_t>(0x07U));
    flags_move.reset(static_cast<uint8_t>(0x02U));
    CASTLE_SAMPLE_CHECK(flags_move.value() == static_cast<uint8_t>(0x05U));

    flags_move.flip<0x01U>();
    CASTLE_SAMPLE_CHECK(flags_move.value() == static_cast<uint8_t>(0x04U));
    flags_move.flip(static_cast<uint8_t>(0x03U));
    CASTLE_SAMPLE_CHECK(flags_move.value() == static_cast<uint8_t>(0x07U));
    flags_move.flip();
    CASTLE_SAMPLE_CHECK(flags_move.value() == static_cast<uint8_t>(0x08U));

    CASTLE_SAMPLE_CHECK(static_cast<uint8_t>(flags_move) == static_cast<uint8_t>(0x08U));

    flags_move.value(static_cast<uint8_t>(0xF2U));
    CASTLE_SAMPLE_CHECK(flags_move.value() == static_cast<uint8_t>(0x02U));

    flags_move |= static_cast<uint8_t>(0x0CU);
    CASTLE_SAMPLE_CHECK(flags_move.value() == static_cast<uint8_t>(0x0EU));
    flags_move &= static_cast<uint8_t>(0x06U);
    CASTLE_SAMPLE_CHECK(flags_move.value() == static_cast<uint8_t>(0x06U));
    flags_move ^= static_cast<uint8_t>(0x03U);
    CASTLE_SAMPLE_CHECK(flags_move.value() == static_cast<uint8_t>(0x05U));

    mode_flags assigned;
    assigned = flags_move;
    CASTLE_SAMPLE_CHECK(assigned == flags_move);
    assigned = static_cast<uint8_t>(0x1FU);
    CASTLE_SAMPLE_CHECK(assigned.value() == static_cast<uint8_t>(0x0FU));

    assigned.clear();
    CASTLE_SAMPLE_CHECK(assigned.none());

    mode_flags lhs(static_cast<uint8_t>(0x01U));
    mode_flags rhs(static_cast<uint8_t>(0x02U));
    lhs.swap(rhs);
    CASTLE_SAMPLE_CHECK(lhs.value() == static_cast<uint8_t>(0x02U));
    CASTLE_SAMPLE_CHECK(rhs.value() == static_cast<uint8_t>(0x01U));
    castle::bit::swap(lhs, rhs);
    CASTLE_SAMPLE_CHECK(lhs.value() == static_cast<uint8_t>(0x01U));
    CASTLE_SAMPLE_CHECK(rhs.value() == static_cast<uint8_t>(0x02U));
    CASTLE_SAMPLE_CHECK(lhs != rhs);
    rhs = static_cast<uint8_t>(0x01U);
    CASTLE_SAMPLE_CHECK(lhs == rhs);

    return 0;
}
