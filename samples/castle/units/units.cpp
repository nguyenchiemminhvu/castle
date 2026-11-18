#include "sample_support.hpp"

#include "castle/units/units.hpp"
#include <stdint.h>

namespace cu = castle::units;
using namespace castle::units::literals;

int main()
{
    // 1. Number in the quantity's own unit
    cu::kilometers<double> leg(1.5);
    double km = leg.value();                                        // 1.5  (kilometers)

    // 2. Number in a chosen unit: convert first, then .value()
    double m1 = cu::meters<double>(leg).value();                    // 1500.0
    double m2 = cu::quantity_cast<cu::meters<double>>(leg).value(); // 1500.0
    cu::meters<double> as_m = leg;                                  // implicit, nothing lost
    double m3 = as_m.value();                                       // 1500.0

    // 3. Integer rep -> double (also rescales)
    cu::millimeters<int32_t> gap(1250);
    double gap_m = cu::meters<double>(gap).value();                 // 1.25

    // 4. Time values are chrono::duration -> .count()
    auto eta = 1.0_km / 25.0_mps;                                   // seconds<double>
    double eta_s  = eta.count();                                    // 40.0
    double eta_ms = cu::milliseconds<double>(eta).count();          // 40000.0

    // 5. Results with no unit are already plain numbers
    double ratio = 500.0_m / 1.0_km;                                // 0.5

    // 6. Passing to a C API that expects radians
    cu::degrees<double> heading(90.0);
    double rad = cu::radians<double>(heading).value();              // 1.5708

    // 7. Temperature: the conversion applies the offset
    double f = cu::fahrenheit<double>(cu::celsius<double>(100.0)).value(); // 212.0

    CASTLE_SAMPLE_CHECK(f == 212.0);
    CASTLE_SAMPLE_CHECK(rad == 3.14159265358979323846 / 2.0);
    CASTLE_SAMPLE_CHECK(ratio == 0.5);
    CASTLE_SAMPLE_CHECK(eta_s == 40.0);
    CASTLE_SAMPLE_CHECK(eta_ms == 40000.0);
    CASTLE_SAMPLE_CHECK(gap_m == 1.25);
    CASTLE_SAMPLE_CHECK(m1 == 1500.0);
    CASTLE_SAMPLE_CHECK(m2 == 1500.0);
    CASTLE_SAMPLE_CHECK(m3 == 1500.0);
    CASTLE_SAMPLE_CHECK(leg.value() == 1.5);
    CASTLE_SAMPLE_CHECK(km == 1.5);

    return 0;
}
