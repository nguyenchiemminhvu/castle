#include "sample_support.hpp"

#include "castle_ext/protocols/rtcm/messages/gps_rtk_observables.hpp"
#include "castle_ext/protocols/rtcm/messages/detail/rtk_observables_family.hpp"

int main()
{
    namespace rtcm = castle::protocols::rtcm;
    using gps_l1_ext = rtcm::messages::detail::rtk_observables_family<rtcm::RTCM_TYPE_GPS_L1_OBS,
                                                                      rtcm::RTCM_TYPE_GPS_L1_OBS_EXT>;

    // Four consecutive numbers: L1, L1 extended, L1/L2, L1/L2 extended. One instantiation is one of them.
    static_assert(gps_l1_ext::FIRST_TYPE == 1001U && gps_l1_ext::LAST_TYPE == 1004U, "GPS family is 1001..1004");
    static_assert(gps_l1_ext::MESSAGE_TYPE == 1002U, "the second number of the family");
    static_assert(gps_l1_ext::IS_EXTENDED && !gps_l1_ext::HAS_L2, "1002 carries ambiguity and CNR, no L2");
    CASTLE_SAMPLE_CHECK(gps_l1_ext::accepts(1002U));
    CASTLE_SAMPLE_CHECK(!gps_l1_ext::accepts(1003U)); // a sibling number of the family
    CASTLE_SAMPLE_CHECK(!gps_l1_ext::accepts(1005U));

    // The message models inherit the constants, so callers ask the model directly.
    using observables = rtcm::messages::gps_rtk_l1_l2_obs_ext;
    static_assert(observables::MESSAGE_TYPE == 1004U, "gps_rtk_l1_l2_obs_ext is message 1004");
    static_assert(observables::IS_EXTENDED && observables::HAS_L2, "1004 carries every optional field");
    static_assert(rtcm::messages::gps_rtk_l1_obs_ext::IS_EXTENDED == gps_l1_ext::IS_EXTENDED
                      && rtcm::messages::gps_rtk_l1_obs_ext::HAS_L2 == gps_l1_ext::HAS_L2,
                  "the 1002 model and the 1002 family instantiation agree");
    CASTLE_SAMPLE_CHECK(observables::accepts(1004U));
    CASTLE_SAMPLE_CHECK(!observables::accepts(1012U)); // the GLONASS counterpart is another family
    return 0;
}
