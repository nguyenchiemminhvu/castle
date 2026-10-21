#include <gtest/gtest.h>

#include "castle_ext/protocols/rtcm/messages/detail/rtk_observables_family.hpp"

namespace
{

using castle::protocols::rtcm::messages::detail::rtk_observables_family;

template <uint16_t Type>
using gps_family = rtk_observables_family<1001U, Type>;
template <uint16_t Type>
using glonass_family = rtk_observables_family<1009U, Type>;

/** @brief Checks the field selection of the message at `Offset` from `First` against the family table. */
template <uint16_t First, uint16_t Offset, bool Extended, bool L2>
CASTLE_CONSTEXPR bool selects()
{
    using family = rtk_observables_family<First, static_cast<uint16_t>(First + Offset)>;
    return (family::MESSAGE_TYPE == First + Offset) && (family::IS_EXTENDED == Extended) && (family::HAS_L2 == L2);
}

TEST(RtcmRtkObservablesFamily, BoundsCoverFourConsecutiveNumbers)
{
    static_assert(gps_family<1001U>::FIRST_TYPE == 1001U, "first number of the GPS family");
    static_assert(gps_family<1001U>::LAST_TYPE == 1004U, "last number of the GPS family");
    static_assert(glonass_family<1009U>::FIRST_TYPE == 1009U, "first number of the GLONASS family");
    static_assert(glonass_family<1009U>::LAST_TYPE == 1012U, "last number of the GLONASS family");
    // The bounds belong to the family, not to the selected message.
    static_assert(gps_family<1004U>::FIRST_TYPE == 1001U, "first number seen from the last message");
    static_assert(glonass_family<1012U>::LAST_TYPE == 1012U, "last number seen from the last message");
    SUCCEED();
}

TEST(RtcmRtkObservablesFamily, OffsetSelectsExtendedAndL2Fields)
{
    // offset: 0 L1, 1 L1 extended, 2 L1/L2, 3 L1/L2 extended.
    static_assert(selects<1001U, 0U, false, false>(), "1001 is plain L1");
    static_assert(selects<1001U, 1U, true, false>(), "1002 is extended L1");
    static_assert(selects<1001U, 2U, false, true>(), "1003 is plain L1/L2");
    static_assert(selects<1001U, 3U, true, true>(), "1004 is extended L1/L2");
    static_assert(selects<1009U, 0U, false, false>(), "1009 is plain L1");
    static_assert(selects<1009U, 1U, true, false>(), "1010 is extended L1");
    static_assert(selects<1009U, 2U, false, true>(), "1011 is plain L1/L2");
    static_assert(selects<1009U, 3U, true, true>(), "1012 is extended L1/L2");
    SUCCEED();
}

/** @brief Expects `Family::accepts()` to hold for `MESSAGE_TYPE` only, across both families and their neighbours. */
template <typename Family>
void expect_accepts_only_its_own_number()
{
    const uint16_t types[] = {0U,    999U,  1000U, 1001U, 1002U, 1003U, 1004U, 1005U, 1006U,
                              1007U, 1008U, 1009U, 1010U, 1011U, 1012U, 1013U, 1014U, 65535U};
    for (const uint16_t type : types)
    {
        EXPECT_EQ(Family::accepts(type), type == Family::MESSAGE_TYPE) << Family::MESSAGE_TYPE << " vs " << type;
    }
}

TEST(RtcmRtkObservablesFamily, AcceptsOnlyTheSelectedNumber)
{
    expect_accepts_only_its_own_number<gps_family<1001U>>();
    expect_accepts_only_its_own_number<gps_family<1002U>>();
    expect_accepts_only_its_own_number<gps_family<1003U>>();
    expect_accepts_only_its_own_number<gps_family<1004U>>();
    expect_accepts_only_its_own_number<glonass_family<1009U>>();
    expect_accepts_only_its_own_number<glonass_family<1010U>>();
    expect_accepts_only_its_own_number<glonass_family<1011U>>();
    expect_accepts_only_its_own_number<glonass_family<1012U>>();
}

TEST(RtcmRtkObservablesFamily, AcceptsIsAConstantExpression)
{
    static_assert(gps_family<1003U>::accepts(1003U), "1003 accepts itself");
    static_assert(!gps_family<1003U>::accepts(1004U), "a sibling number is rejected");
    static_assert(!glonass_family<1011U>::accepts(1003U), "the same offset in the other family is rejected");
    SUCCEED();
}

} // namespace
