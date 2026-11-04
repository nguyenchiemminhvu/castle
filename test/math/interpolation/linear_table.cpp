
#include <gtest/gtest.h>

#include <limits>

#include "castle/math/interpolation/linear_table.hpp"

namespace
{

using castle::size_type;
using castle::status;
using castle::math::extrapolation;
using castle::math::linear_table;

// Knots: (0,0) (1,2) (3,2) (4,-2): rising, flat, falling segments.
template <typename T>
void check_default_and_bind(T tol)
{
    const T xs[] = {0, 1, 3, 4};
    const T ys[] = {0, 2, 2, -2};

    linear_table<T> table;
    EXPECT_FALSE(table.valid());
    EXPECT_EQ(table.size(), 0U);

    EXPECT_EQ(table.assign(xs, ys, 4U), status::ok);
    EXPECT_TRUE(table.valid());
    EXPECT_EQ(table.size(), 4U);
    EXPECT_NEAR(table.x_min(), 0, tol);
    EXPECT_NEAR(table.x_max(), 4, tol);

    // Constructor binds like assign().
    const linear_table<T> bound(xs, ys, 4U);
    EXPECT_TRUE(bound.valid());
    EXPECT_EQ(bound.size(), 4U);
    EXPECT_NEAR(bound.x_min(), 0, tol);
    EXPECT_NEAR(bound.x_max(), 4, tol);

    const linear_table<T> invalid_ctor(xs, ys, 1U);
    EXPECT_FALSE(invalid_ctor.valid());
    EXPECT_EQ(invalid_ctor.size(), 0U);
}

template <typename T>
void check_evaluate(T tol)
{
    const T xs[] = {0, 1, 3, 4};
    const T ys[] = {0, 2, 2, -2};
    const linear_table<T> table(xs, ys, 4U);
    ASSERT_TRUE(table.valid());

    // Knots are reproduced exactly (clamp and linear).
    for (size_type i = 0U; i < 4U; ++i)
    {
        EXPECT_NEAR(table.evaluate(xs[i]), ys[i], tol);
        EXPECT_NEAR(table.evaluate(xs[i], extrapolation::linear), ys[i], tol);
        EXPECT_NEAR(table(xs[i]), ys[i], tol);
    }

    // Interior values.
    EXPECT_NEAR(table.evaluate(static_cast<T>(0.5)), 1, tol);
    EXPECT_NEAR(table.evaluate(static_cast<T>(2)), 2, tol);
    EXPECT_NEAR(table.evaluate(static_cast<T>(3.5)), 0, tol);
    EXPECT_NEAR(table(static_cast<T>(3.25)), 1, tol);

    // Below: clamp holds, linear extends first segment (slope 2).
    EXPECT_NEAR(table.evaluate(-1), 0, tol);
    EXPECT_NEAR(table.evaluate(-1, extrapolation::clamp), 0, tol);
    EXPECT_NEAR(table.evaluate(-1, extrapolation::linear), -2, tol);

    // Above: clamp holds, linear extends last segment (slope -4).
    EXPECT_NEAR(table.evaluate(5), -2, tol);
    EXPECT_NEAR(table.evaluate(5, extrapolation::clamp), -2, tol);
    EXPECT_NEAR(table.evaluate(5, extrapolation::linear), -6, tol);
    EXPECT_NEAR(table(100), -2, tol);
}

template <typename T>
void check_derivative(T tol)
{
    const T xs[] = {0, 1, 3, 4};
    const T ys[] = {0, 2, 2, -2};
    const linear_table<T> table(xs, ys, 4U);
    ASSERT_TRUE(table.valid());

    // Inside: slope of the containing segment, right segment at interior knots.
    EXPECT_NEAR(table.derivative(static_cast<T>(0.5)), 2, tol);
    EXPECT_NEAR(table.derivative(0), 2, tol);
    EXPECT_NEAR(table.derivative(1), 0, tol);
    EXPECT_NEAR(table.derivative(2), 0, tol);
    EXPECT_NEAR(table.derivative(3), -4, tol);
    EXPECT_NEAR(table.derivative(static_cast<T>(3.5)), -4, tol);
    EXPECT_NEAR(table.derivative(4), -4, tol);
    EXPECT_NEAR(table.derivative(2, extrapolation::linear), 0, tol);

    // Below the table.
    EXPECT_NEAR(table.derivative(-1), 0, tol);
    EXPECT_NEAR(table.derivative(-1, extrapolation::clamp), 0, tol);
    EXPECT_NEAR(table.derivative(-1, extrapolation::linear), 2, tol);

    // Above the table.
    EXPECT_NEAR(table.derivative(9), 0, tol);
    EXPECT_NEAR(table.derivative(9, extrapolation::clamp), 0, tol);
    EXPECT_NEAR(table.derivative(9, extrapolation::linear), -4, tol);
}

template <typename T>
void check_two_knots(T tol)
{
    const T xs[] = {1, 3};
    const T ys[] = {10, 20};
    const linear_table<T> table(xs, ys, 2U);
    ASSERT_TRUE(table.valid());
    EXPECT_EQ(table.size(), 2U);
    EXPECT_NEAR(table.evaluate(1), 10, tol);
    EXPECT_NEAR(table.evaluate(2), 15, tol);
    EXPECT_NEAR(table.evaluate(3), 20, tol);
    EXPECT_NEAR(table.evaluate(0), 10, tol);
    EXPECT_NEAR(table.evaluate(0, extrapolation::linear), 5, tol);
    EXPECT_NEAR(table.evaluate(4, extrapolation::linear), 25, tol);
    EXPECT_NEAR(table.derivative(2), 5, tol);
    EXPECT_NEAR(table.derivative(0, extrapolation::linear), 5, tol);
    EXPECT_NEAR(table.derivative(4, extrapolation::linear), 5, tol);
    EXPECT_NEAR(table.derivative(4), 0, tol);
}

template <typename T>
void check_assign_failures()
{
    const T xs[] = {0, 1, 2};
    const T ys[] = {5, 6, 7};
    const T flat[] = {0, 1, 1};
    const T decreasing[] = {0, 2, 1};
    const T nan_x[] = {0, std::numeric_limits<T>::quiet_NaN(), 2};

    linear_table<T> table;
    ASSERT_EQ(table.assign(xs, ys, 3U), status::ok);
    ASSERT_TRUE(table.valid());

    // Null ys: failure unbinds the table.
    EXPECT_EQ(table.assign(xs, nullptr, 3U), status::invalid_argument);
    EXPECT_FALSE(table.valid());
    EXPECT_EQ(table.size(), 0U);

    // Null xs.
    ASSERT_EQ(table.assign(xs, ys, 3U), status::ok);
    EXPECT_EQ(table.assign(nullptr, ys, 3U), status::invalid_argument);
    EXPECT_FALSE(table.valid());

    // Both null.
    EXPECT_EQ(table.assign(nullptr, nullptr, 3U), status::invalid_argument);
    EXPECT_FALSE(table.valid());

    // Too few knots.
    ASSERT_EQ(table.assign(xs, ys, 3U), status::ok);
    EXPECT_EQ(table.assign(xs, ys, 1U), status::invalid_argument);
    EXPECT_FALSE(table.valid());
    EXPECT_EQ(table.assign(xs, ys, 0U), status::invalid_argument);
    EXPECT_FALSE(table.valid());

    // Non-monotone abscissas.
    ASSERT_EQ(table.assign(xs, ys, 3U), status::ok);
    EXPECT_EQ(table.assign(flat, ys, 3U), status::invalid_argument);
    EXPECT_FALSE(table.valid());
    EXPECT_EQ(table.assign(decreasing, ys, 3U), status::invalid_argument);
    EXPECT_FALSE(table.valid());
    EXPECT_EQ(table.assign(nan_x, ys, 3U), status::invalid_argument);
    EXPECT_FALSE(table.valid());

    // Rebuild after failure.
    EXPECT_EQ(table.assign(xs, ys, 3U), status::ok);
    EXPECT_TRUE(table.valid());
    EXPECT_EQ(table.size(), 3U);

    // Rebinding to a shorter prefix keeps the table consistent.
    EXPECT_EQ(table.assign(xs, ys, 2U), status::ok);
    EXPECT_EQ(table.size(), 2U);
    EXPECT_EQ(table.x_max(), static_cast<T>(1));

    // Constructor with every failure input leaves the table unbound.
    EXPECT_FALSE(linear_table<T>(xs, nullptr, 3U).valid());
    EXPECT_FALSE(linear_table<T>(nullptr, ys, 3U).valid());
    EXPECT_FALSE(linear_table<T>(flat, ys, 3U).valid());
}

TEST(MathLinearTable, DefaultAndBindFloat)
{
    check_default_and_bind<float>(1e-6F);
}

TEST(MathLinearTable, DefaultAndBindDouble)
{
    check_default_and_bind<double>(1e-12);
}

TEST(MathLinearTable, EvaluateFloat)
{
    check_evaluate<float>(1e-5F);
}

TEST(MathLinearTable, EvaluateDouble)
{
    check_evaluate<double>(1e-12);
}

TEST(MathLinearTable, DerivativeFloat)
{
    check_derivative<float>(1e-5F);
}

TEST(MathLinearTable, DerivativeDouble)
{
    check_derivative<double>(1e-12);
}

TEST(MathLinearTable, TwoKnotsFloat)
{
    check_two_knots<float>(1e-5F);
}

TEST(MathLinearTable, TwoKnotsDouble)
{
    check_two_knots<double>(1e-12);
}

TEST(MathLinearTable, AssignFailuresFloat)
{
    check_assign_failures<float>();
}

TEST(MathLinearTable, AssignFailuresDouble)
{
    check_assign_failures<double>();
}

TEST(MathLinearTable, TableObservesBoundArrays)
{
    double ys[] = {0.0, 10.0, 20.0};
    const double xs[] = {0.0, 1.0, 2.0};
    const linear_table<double> table(xs, ys, 3U);
    EXPECT_NEAR(table.evaluate(0.5), 5.0, 1e-12);
    ys[0] = 10.0;
    EXPECT_NEAR(table.evaluate(0.5), 10.0, 1e-12);
}

TEST(MathLinearTable, ConstexprDefaultConstruction)
{
    static constexpr linear_table<double> table;
    static_assert(!table.valid(), "linear_table");
    static_assert(table.size() == 0U, "linear_table");
    EXPECT_FALSE(table.valid());
}

TEST(MathLinearTable, ConstexprConstructionAndEvaluation)
{
    static constexpr double xs[] = {0.0, 1.0, 2.0, 4.0};
    static constexpr double ys[] = {0.0, 2.0, 4.0, 5.0};
    static constexpr linear_table<double> table(xs, ys, 4U);
    static_assert(table.valid(), "linear_table");
    static_assert(table.evaluate(3.0) == 4.5, "linear_table");
    static_assert(table.derivative(1.5) == 2.0, "linear_table");
    EXPECT_TRUE(table.valid());
}

} // namespace
