#include "sample_support.hpp"

#include "castle/math/abs.hpp"
#include "castle/math/complex.hpp"
#include "castle/math/linalg/vector.hpp"
#include "castle/math/near_equal.hpp"
#include "castle/math/numeric/ode.hpp"

int main()
{
    // y' = -y, y(0) = 1  ->  y(1) = e^-1.
    const auto decay = [](double, double y) { return -y; };
    const double exact = 0.36787944117144233;

    double y_euler = 1.0;
    double y_rk4 = 1.0;
    for (int i = 0; i < 10; ++i)
    {
        const double t = 0.1 * static_cast<double>(i);
        y_euler = castle::math::euler_step(decay, t, y_euler, 0.1);
        y_rk4 = castle::math::rk4_step(decay, t, y_rk4, 0.1);
    }
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(y_rk4, exact, 1.0e-6));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(y_euler, exact, 0.05));
    CASTLE_SAMPLE_CHECK(castle::math::abs(y_rk4 - exact) < castle::math::abs(y_euler - exact));

    // A single Euler step is y + h * f(t, y).
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::euler_step(decay, 0.0, 1.0, 0.5), 0.5, 1.0e-12));

    // The step size may be negative to integrate backwards.
    const double back = castle::math::rk4_step(decay, 1.0, y_rk4, -1.0);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(back, 1.0, 1.0e-2));

    // Vector state: harmonic oscillator x'' = -x written as (x, v)' = (v, -x).
    typedef castle::math::vector<double, 2U> state_type;
    const auto oscillator = [](double, const state_type& s) { return state_type(s[1U], -s[0U]); };
    state_type s(1.0, 0.0);
    for (int i = 0; i < 100; ++i)
    {
        s = castle::math::rk4_step(oscillator, 0.01 * static_cast<double>(i), s, 0.01);
    }
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(s[0U], 0.5403023058681398, 1.0e-8));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(s[1U], -0.8414709848078965, 1.0e-8));

    // Complex state: z' = i z rotates the unit phasor.
    typedef castle::math::complex<double> phasor;
    const auto rotation = [](double, const phasor& z) { return phasor(0.0, 1.0) * z; };
    phasor z(1.0, 0.0);
    for (int i = 0; i < 100; ++i)
    {
        z = castle::math::rk4_step(rotation, 0.01 * static_cast<double>(i), z, 0.01);
    }
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(z.real(), 0.5403023058681398, 1.0e-8));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(z.imag(), 0.8414709848078965, 1.0e-8));

    // Time-dependent right-hand side and single precision: y' = t, y(0) = 0 -> y(1) = 0.5.
    const auto ramp = [](float t, float) { return t; };
    float yf = 0.0F;
    for (int i = 0; i < 10; ++i)
    {
        yf = castle::math::rk4_step(ramp, 0.1F * static_cast<float>(i), yf, 0.1F);
    }
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(yf, 0.5F, 1.0e-5F));

    return 0;
}
