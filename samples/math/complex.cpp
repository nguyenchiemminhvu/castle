#include "sample_support.hpp"

#include "castle/core/constants.hpp"
#include "castle/math/complex.hpp"
#include "castle/math/linalg/trigonometry.hpp"
#include "castle/math/near_equal.hpp"

int main()
{
    using castle::math::complexd;
    using castle::math::complexf;

    // Construction, accessors and setters
    const complexf zero;
    CASTLE_SAMPLE_CHECK(zero.real() == 0.0F && zero.imag() == 0.0F);
    complexf z(3.0F, 4.0F);
    CASTLE_SAMPLE_CHECK(z.real() == 3.0F && z.imag() == 4.0F);
    CASTLE_SAMPLE_CHECK(castle::math::real(z) == 3.0F && castle::math::imag(z) == 4.0F);
    z.real(1.0F);
    z.imag(-2.0F);
    CASTLE_SAMPLE_CHECK(z == complexf(1.0F, -2.0F));
    const complexf from_real(2.5F);
    CASTLE_SAMPLE_CHECK(from_real == complexf(2.5F, 0.0F));

    // Explicit conversion between component types
    const complexd wide(z);
    CASTLE_SAMPLE_CHECK(wide.real() == 1.0 && wide.imag() == -2.0);
    const complexf narrow(complexd(0.5, 0.25));
    CASTLE_SAMPLE_CHECK(narrow == complexf(0.5F, 0.25F));

    // Unary operators and real assignment / compound assignment
    CASTLE_SAMPLE_CHECK(+z == z);
    CASTLE_SAMPLE_CHECK(-z == complexf(-1.0F, 2.0F));
    complexf acc(1.0F, 1.0F);
    acc = 5.0F;
    CASTLE_SAMPLE_CHECK(acc == complexf(5.0F, 0.0F));
    acc += 1.0F;
    acc -= 2.0F;
    acc *= 3.0F;
    acc /= 2.0F;
    CASTLE_SAMPLE_CHECK(acc == complexf(6.0F, 0.0F));
    acc += complexf(1.0F, 2.0F);
    acc -= complexf(0.0F, 1.0F);
    CASTLE_SAMPLE_CHECK(acc == complexf(7.0F, 1.0F));
    acc *= complexf(0.0F, 1.0F); // multiply by i
    CASTLE_SAMPLE_CHECK(acc == complexf(-1.0F, 7.0F));
    acc /= complexf(0.0F, 1.0F); // divide by i
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(acc, complexf(7.0F, 1.0F), 1.0e-6F));

    // Binary arithmetic with complex and real operands
    const complexd a(1.0, 2.0);
    const complexd b(3.0, -1.0);
    CASTLE_SAMPLE_CHECK(a + b == complexd(4.0, 1.0));
    CASTLE_SAMPLE_CHECK(a + 1.0 == complexd(2.0, 2.0));
    CASTLE_SAMPLE_CHECK(1.0 + a == complexd(2.0, 2.0));
    CASTLE_SAMPLE_CHECK(a - b == complexd(-2.0, 3.0));
    CASTLE_SAMPLE_CHECK(a - 1.0 == complexd(0.0, 2.0));
    CASTLE_SAMPLE_CHECK(1.0 - a == complexd(0.0, -2.0));
    CASTLE_SAMPLE_CHECK(a * b == complexd(5.0, 5.0));
    CASTLE_SAMPLE_CHECK(a * 2.0 == complexd(2.0, 4.0));
    CASTLE_SAMPLE_CHECK(2.0 * a == complexd(2.0, 4.0));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(a / b, complexd(0.1, 0.7), 1.0e-12));
    CASTLE_SAMPLE_CHECK(a / 2.0 == complexd(0.5, 1.0));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(2.0 / a, complexd(0.4, -0.8), 1.0e-12));

    // Equality and inequality against complex and real operands
    CASTLE_SAMPLE_CHECK(a == a);
    CASTLE_SAMPLE_CHECK(a != b);
    CASTLE_SAMPLE_CHECK(complexd(2.0, 0.0) == 2.0);
    CASTLE_SAMPLE_CHECK(a != 1.0);

    // Conjugate, norm, magnitude, phase
    const complexd c(3.0, 4.0);
    CASTLE_SAMPLE_CHECK(castle::math::conj(c) == complexd(3.0, -4.0));
    CASTLE_SAMPLE_CHECK(castle::math::norm(c) == 25.0);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::abs(c), 5.0, 1.0e-12));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::arg(complexd(0.0, 1.0)),
                                                 castle::math::pi<double>() * 0.5, 1.0e-9));

    // Polar construction round trip
    const complexd p = castle::math::polar(2.0, 0.5);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::abs(p), 2.0, 1.0e-9));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::arg(p), 0.5, 1.0e-9));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::polar(3.0), complexd(3.0, 0.0), 1.0e-12));

    // Unit-magnitude phase factor
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::cis(castle::math::pi<double>() * 0.5),
                                                 complexd(0.0, 1.0), 1.0e-9));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::cis(0.5), castle::math::polar(1.0, 0.5), 1.0e-12));

    // Exponential and logarithm
    const complexd euler = castle::math::exp(complexd(0.0, castle::math::pi<double>()));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(euler, complexd(-1.0, 0.0), 1.0e-9));
    const complexd lg = castle::math::log(complexd(-1.0, 0.0));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(lg, complexd(0.0, castle::math::pi<double>()), 1.0e-9));

    // Principal square root
    CASTLE_SAMPLE_CHECK(castle::math::sqrt(complexd()) == complexd());
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::sqrt(complexd(-4.0, 0.0)),
                                                 complexd(0.0, 2.0), 1.0e-9));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::sqrt(complexd(-3.0, -4.0)),
                                                 complexd(1.0, -2.0), 1.0e-9));

    // Integer powers
    const complexd i_unit(0.0, 1.0);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::pow(i_unit, 2), complexd(-1.0, 0.0), 1.0e-12));
    CASTLE_SAMPLE_CHECK(castle::math::pow(i_unit, 0) == complexd(1.0, 0.0));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::pow(complexd(0.0, 2.0), -2),
                                                 complexd(-0.25, 0.0), 1.0e-12));

    // Complex tolerance comparison
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(complexd(1.0, 1.0), complexd(1.0, 1.0 + 1.0e-10), 1.0e-9));
    CASTLE_SAMPLE_CHECK(!castle::math::near_equal(complexd(1.0, 1.0), complexd(1.0, 1.1), 1.0e-9));

    return 0;
}
