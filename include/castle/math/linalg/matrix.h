#ifndef CASTLE_MATH_MATRIX_H
#define CASTLE_MATH_MATRIX_H

#include "castle/core/compiler.h"
#include "castle/core/error_handler.h"
#include "castle/core/traits.h"
#include "castle/core/types.h"
#include "castle/math/abs.h"
#include "castle/math/near_equal.h"
#include "castle/math/linalg/vector.h"

namespace castle
{
namespace math
{

// ============================================================================
// Fixed-size row-major mathematical matrix.
//
// Multiplication follows the conventional column-vector model:
//     result = matrix * vector
// and homogeneous transform matrices therefore store translation in the
// final column. Storage is fully embedded and deterministic.
// ============================================================================
template <typename T, size_type Rows, size_type Columns>
class matrix
{
    static_assert(Rows > 0U, "math::matrix requires a positive row count");
    static_assert(Columns > 0U, "math::matrix requires a positive column count");
    static_assert(meta::is_arithmetic<T>::value,
                  "math::matrix requires an arithmetic type");

public:
    using value_type = T;
    using size_type = castle::size_type;
    using reference = T&;
    using const_reference = CASTLE_CONST T&;
    using pointer = T*;
    using const_pointer = CASTLE_CONST T*;
    using iterator = T*;
    using const_iterator = CASTLE_CONST T*;

    static CASTLE_CONSTEXPR size_type static_rows = Rows;
    static CASTLE_CONSTEXPR size_type static_columns = Columns;

    CASTLE_CONSTEXPR matrix() CASTLE_NOEXCEPT : data_{}
    {
    }

    template <typename... Args,
              meta::enable_if_t<(sizeof...(Args) == Rows * Columns) &&
                                meta::conjunction<meta::is_constructible<T, Args&&>...>::value, int> = 0>
    CASTLE_CONSTEXPR explicit matrix(Args&&... args)
        : data_{static_cast<T>(args)...}
    {
    }

    CASTLE_CONSTEXPR size_type rows() CASTLE_CONST CASTLE_NOEXCEPT { return Rows; }
    CASTLE_CONSTEXPR size_type columns() CASTLE_CONST CASTLE_NOEXCEPT { return Columns; }
    CASTLE_CONSTEXPR size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return Rows * Columns; }

    CASTLE_CONSTEXPR iterator begin() CASTLE_NOEXCEPT { return data_; }
    CASTLE_CONSTEXPR const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }
    CASTLE_CONSTEXPR const_iterator cbegin() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }
    CASTLE_CONSTEXPR iterator end() CASTLE_NOEXCEPT { return data_ + Rows * Columns; }
    CASTLE_CONSTEXPR const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return data_ + Rows * Columns; }
    CASTLE_CONSTEXPR const_iterator cend() CASTLE_CONST CASTLE_NOEXCEPT { return data_ + Rows * Columns; }

    CASTLE_CONSTEXPR reference operator()(size_type row, size_type column) CASTLE_NOEXCEPT
    {
        return data_[row * Columns + column];
    }

    CASTLE_CONSTEXPR const_reference operator()(size_type row, size_type column) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return data_[row * Columns + column];
    }

    CASTLE_CONSTEXPR reference at(size_type row, size_type column) CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(row < Rows && column < Columns,
                      CASTLE_ERROR_GENERIC("castle::math::matrix::at: index out of range"));
        return data_[row * Columns + column];
    }

    CASTLE_CONSTEXPR const_reference at(size_type row, size_type column) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(row < Rows && column < Columns,
                      CASTLE_ERROR_GENERIC("castle::math::matrix::at: index out of range"));
        return data_[row * Columns + column];
    }

    CASTLE_CONSTEXPR pointer data() CASTLE_NOEXCEPT { return data_; }
    CASTLE_CONSTEXPR const_pointer data() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }

    CASTLE_CONSTEXPR matrix operator+() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return *this;
    }

    CASTLE_CONSTEXPR matrix operator-() CASTLE_CONST CASTLE_NOEXCEPT
    {
        matrix result;
        for (size_type i = 0U; i < Rows * Columns; ++i)
        {
            result.data_[i] = static_cast<T>(-data_[i]);
        }
        return result;
    }

    CASTLE_CONSTEXPR matrix operator+(CASTLE_CONST matrix& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        matrix result;
        for (size_type i = 0U; i < Rows * Columns; ++i)
        {
            result.data_[i] = static_cast<T>(data_[i] + other.data_[i]);
        }
        return result;
    }

    CASTLE_CONSTEXPR matrix operator-(CASTLE_CONST matrix& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        matrix result;
        for (size_type i = 0U; i < Rows * Columns; ++i)
        {
            result.data_[i] = static_cast<T>(data_[i] - other.data_[i]);
        }
        return result;
    }

    CASTLE_CONSTEXPR matrix& operator+=(CASTLE_CONST matrix& other) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < Rows * Columns; ++i)
        {
            data_[i] = static_cast<T>(data_[i] + other.data_[i]);
        }
        return *this;
    }

    CASTLE_CONSTEXPR matrix& operator-=(CASTLE_CONST matrix& other) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < Rows * Columns; ++i)
        {
            data_[i] = static_cast<T>(data_[i] - other.data_[i]);
        }
        return *this;
    }

    CASTLE_CONSTEXPR matrix operator*(T scalar) CASTLE_CONST CASTLE_NOEXCEPT
    {
        matrix result;
        for (size_type i = 0U; i < Rows * Columns; ++i)
        {
            result.data_[i] = static_cast<T>(data_[i] * scalar);
        }
        return result;
    }

    matrix operator/(T scalar) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(scalar != static_cast<T>(0),
                      CASTLE_ERROR_GENERIC("castle::math::matrix: division by zero"));

        matrix result;
        for (size_type i = 0U; i < Rows * Columns; ++i)
        {
            result.data_[i] = static_cast<T>(data_[i] / scalar);
        }
        return result;
    }

    CASTLE_CONSTEXPR matrix& operator*=(T scalar) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < Rows * Columns; ++i)
        {
            data_[i] = static_cast<T>(data_[i] * scalar);
        }
        return *this;
    }

    matrix& operator/=(T scalar) CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(scalar != static_cast<T>(0),
                      CASTLE_ERROR_GENERIC("castle::math::matrix: division by zero"));

        for (size_type i = 0U; i < Rows * Columns; ++i)
        {
            data_[i] = static_cast<T>(data_[i] / scalar);
        }
        return *this;
    }

    CASTLE_CONSTEXPR bool operator==(CASTLE_CONST matrix& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < Rows * Columns; ++i)
        {
            if (data_[i] != other.data_[i])
            {
                return false;
            }
        }
        return true;
    }

    CASTLE_CONSTEXPR bool operator!=(CASTLE_CONST matrix& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return !(*this == other);
    }

    CASTLE_CONSTEXPR vector<T, Columns> row(size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(index < Rows,
                      CASTLE_ERROR_GENERIC("castle::math::matrix::row: index out of range"));
        vector<T, Columns> result;
        for (size_type i = 0U; i < Columns; ++i)
        {
            result[i] = data_[index * Columns + i];
        }
        return result;
    }

    CASTLE_CONSTEXPR vector<T, Rows> column(size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(index < Columns,
                      CASTLE_ERROR_GENERIC("castle::math::matrix::column: index out of range"));
        vector<T, Rows> result;
        for (size_type i = 0U; i < Rows; ++i)
        {
            result[i] = data_[i * Columns + index];
        }
        return result;
    }

    CASTLE_CONSTEXPR matrix<T, Columns, Rows> transpose() CASTLE_CONST CASTLE_NOEXCEPT
    {
        matrix<T, Columns, Rows> result;
        for (size_type row = 0U; row < Rows; ++row)
        {
            for (size_type column = 0U; column < Columns; ++column)
            {
                result(column, row) = data_[row * Columns + column];
            }
        }
        return result;
    }

    CASTLE_CONSTEXPR T trace() CASTLE_CONST CASTLE_NOEXCEPT
    {
        static_assert(Rows == Columns, "matrix::trace requires a square matrix");
        T result{};
        for (size_type i = 0U; i < Rows; ++i)
        {
            result = static_cast<T>(result + data_[i * Columns + i]);
        }
        return result;
    }

private:
    T data_[Rows * Columns];
};

template <typename T, size_type Rows, size_type Columns>
CASTLE_CONSTEXPR matrix<T, Rows, Columns> operator*(
    T scalar,
    CASTLE_CONST matrix<T, Rows, Columns>& value) CASTLE_NOEXCEPT
{
    return value * scalar;
}

template <typename T, size_type Rows, size_type Columns, typename U>
CASTLE_NODISCARD CASTLE_CONSTEXPR matrix<typename meta::common_type_t<T, U>, Rows, Columns> operator+(
    CASTLE_CONST matrix<T, Rows, Columns>& first,
    CASTLE_CONST matrix<U, Rows, Columns>& second) CASTLE_NOEXCEPT
{
    using result_type = typename meta::common_type<T, U>::type;
    matrix<result_type, Rows, Columns> result;
    for (size_type r = 0U; r < Rows; ++r)
    {
        for (size_type c = 0U; c < Columns; ++c)
        {
            result(r, c) = static_cast<result_type>(first(r, c) + second(r, c));
        }
    }
    return result;
}

template <typename T, size_type Rows, size_type Columns, typename U>
CASTLE_NODISCARD CASTLE_CONSTEXPR matrix<typename meta::common_type_t<T, U>, Rows, Columns> operator-(
    CASTLE_CONST matrix<T, Rows, Columns>& first,
    CASTLE_CONST matrix<U, Rows, Columns>& second) CASTLE_NOEXCEPT
{
    using result_type = typename meta::common_type<T, U>::type;
    matrix<result_type, Rows, Columns> result;
    for (size_type r = 0U; r < Rows; ++r)
    {
        for (size_type c = 0U; c < Columns; ++c)
        {
            result(r, c) = static_cast<result_type>(first(r, c) - second(r, c));
        }
    }
    return result;
}

template <typename T, size_type Rows, size_type Columns, typename U, size_type OtherColumns>
CASTLE_NODISCARD CASTLE_CONSTEXPR matrix<typename meta::common_type_t<T, U>, Rows, OtherColumns> operator*(
    CASTLE_CONST matrix<T, Rows, Columns>& first,
    CASTLE_CONST matrix<U, Columns, OtherColumns>& second) CASTLE_NOEXCEPT
{
    using result_type = typename meta::common_type<T, U>::type;
    matrix<result_type, Rows, OtherColumns> result;

    for (size_type r = 0U; r < Rows; ++r)
    {
        for (size_type c = 0U; c < OtherColumns; ++c)
        {
            result(r, c) = result_type{};
            for (size_type k = 0U; k < Columns; ++k)
            {
                result(r, c) = static_cast<result_type>(
                    result(r, c) + static_cast<result_type>(first(r, k)) * static_cast<result_type>(second(k, c)));
            }
        }
    }
    return result;
}

template <typename T, size_type Rows, size_type Columns>
CASTLE_NODISCARD CASTLE_CONSTEXPR vector<T, Rows> operator*(
    CASTLE_CONST matrix<T, Rows, Columns>& value,
    CASTLE_CONST vector<T, Columns>& input) CASTLE_NOEXCEPT
{
    vector<T, Rows> result;
    for (size_type r = 0U; r < Rows; ++r)
    {
        result[r] = T{};
        for (size_type c = 0U; c < Columns; ++c)
        {
            result[r] = static_cast<T>(result[r] + value(r, c) * input[c]);
        }
    }
    return result;
}

template <typename T, size_type Rows, size_type Columns, typename U>
CASTLE_NODISCARD CASTLE_CONSTEXPR vector<typename meta::common_type_t<T, U>, Rows> operator*(
    CASTLE_CONST matrix<T, Rows, Columns>& value,
    CASTLE_CONST vector<U, Columns>& input) CASTLE_NOEXCEPT
{
    using result_type = typename meta::common_type<T, U>::type;
    vector<result_type, Rows> result;
    for (size_type r = 0U; r < Rows; ++r)
    {
        result[r] = result_type{};
        for (size_type c = 0U; c < Columns; ++c)
        {
            result[r] = static_cast<result_type>(result[r] +
                                                  static_cast<result_type>(value(r, c)) *
                                                  static_cast<result_type>(input[c]));
        }
    }
    return result;
}

template <typename T, size_type Rows, size_type Columns>
CASTLE_CONSTEXPR matrix<T, Columns, Rows> transpose(
    CASTLE_CONST matrix<T, Rows, Columns>& value) CASTLE_NOEXCEPT
{
    matrix<T, Columns, Rows> result;
    for (size_type r = 0U; r < Rows; ++r)
    {
        for (size_type c = 0U; c < Columns; ++c)
        {
            result(c, r) = value(r, c);
        }
    }
    return result;
}

template <typename T, size_type Rows, size_type Columns>
CASTLE_CONSTEXPR matrix<T, Rows, Columns> hadamard(
    CASTLE_CONST matrix<T, Rows, Columns>& first,
    CASTLE_CONST matrix<T, Rows, Columns>& second) CASTLE_NOEXCEPT
{
    matrix<T, Rows, Columns> result;
    for (size_type row = 0U; row < Rows; ++row)
    {
        for (size_type column = 0U; column < Columns; ++column)
        {
            result(row, column) = static_cast<T>(first(row, column) * second(row, column));
        }
    }
    return result;
}

template <typename T, size_type Rows, size_type Columns>
CASTLE_CONSTEXPR T frobenius_squared(
    CASTLE_CONST matrix<T, Rows, Columns>& value) CASTLE_NOEXCEPT
{
    T result{};
    for (size_type row = 0U; row < Rows; ++row)
    {
        for (size_type column = 0U; column < Columns; ++column)
        {
            result = static_cast<T>(result + value(row, column) * value(row, column));
        }
    }
    return result;
}

template <typename T, size_type N>
CASTLE_CONSTEXPR matrix<T, N, N> diagonal_matrix(
    CASTLE_CONST vector<T, N>& values) CASTLE_NOEXCEPT
{
    matrix<T, N, N> result;
    for (size_type i = 0U; i < N; ++i)
    {
        result(i, i) = values[i];
    }
    return result;
}

template <typename T>
CASTLE_CONSTEXPR matrix<T, 3U, 3U> skew_symmetric(
    CASTLE_CONST vector<T, 3U>& value) CASTLE_NOEXCEPT
{
    return matrix<T, 3U, 3U>(
        static_cast<T>(0), static_cast<T>(-value[2U]), value[1U],
        value[2U], static_cast<T>(0), static_cast<T>(-value[0U]),
        static_cast<T>(-value[1U]), value[0U], static_cast<T>(0));
}

template <typename T, size_type N>
CASTLE_CONSTEXPR matrix<T, N, N> outer_product(
    CASTLE_CONST vector<T, N>& first,
    CASTLE_CONST vector<T, N>& second) CASTLE_NOEXCEPT
{
    matrix<T, N, N> result;
    for (size_type row = 0U; row < N; ++row)
    {
        for (size_type column = 0U; column < N; ++column)
        {
            result(row, column) = static_cast<T>(first[row] * second[column]);
        }
    }
    return result;
}

template <typename T>
using matrix2x2 = matrix<T, 2U, 2U>;

template <typename T>
using matrix3x3 = matrix<T, 3U, 3U>;

template <typename T>
using matrix4x4 = matrix<T, 4U, 4U>;

// ============================================================================
// Determinant helper.
//
// The recursive cofactor form avoids division, so integer matrices remain
// supported. It is intended for small fixed matrices (2x2 through 4x4 are the
// primary embedded use cases).
// ============================================================================
template <typename T, size_type N>
struct determinant_helper;

template <typename T>
struct determinant_helper<T, 1U>
{
    static CASTLE_CONSTEXPR T compute(CASTLE_CONST matrix<T, 1U, 1U>& value) CASTLE_NOEXCEPT
    {
        return value(0U, 0U);
    }
};

template <typename T>
struct determinant_helper<T, 2U>
{
    static CASTLE_CONSTEXPR T compute(CASTLE_CONST matrix<T, 2U, 2U>& value) CASTLE_NOEXCEPT
    {
        return static_cast<T>(value(0U, 0U) * value(1U, 1U) -
                              value(0U, 1U) * value(1U, 0U));
    }
};

template <typename T, size_type N>
struct determinant_helper
{
    static T compute(CASTLE_CONST matrix<T, N, N>& value) CASTLE_NOEXCEPT
    {
        T result{};
        for (size_type column = 0U; column < N; ++column)
        {
            matrix<T, N - 1U, N - 1U> minor;
            size_type minor_row = 0U;
            for (size_type row = 1U; row < N; ++row)
            {
                size_type minor_column = 0U;
                for (size_type current_column = 0U; current_column < N; ++current_column)
                {
                    if (current_column == column)
                    {
                        continue;
                    }
                    minor(minor_row, minor_column) = value(row, current_column);
                    ++minor_column;
                }
                ++minor_row;
            }

            CASTLE_CONST T cofactor = determinant_helper<T, N - 1U>::compute(minor);
            CASTLE_CONST T signed_term = ((column & 1U) == 0U)
                                  ? static_cast<T>(value(0U, column) * cofactor)
                                  : static_cast<T>(-value(0U, column) * cofactor);
            result = static_cast<T>(result + signed_term);
        }
        return result;
    }
};

template <typename T, size_type N>
CASTLE_NODISCARD T determinant(CASTLE_CONST matrix<T, N, N>& value) CASTLE_NOEXCEPT
{
    return determinant_helper<T, N>::compute(value);
}

template <typename T, size_type N>
CASTLE_NODISCARD bool try_inverse(
    CASTLE_CONST matrix<T, N, N>& value,
    matrix<T, N, N>& result,
    T epsilon = meta::floating_epsilon<T>::value) CASTLE_NOEXCEPT
{
    static_assert(meta::is_floating_point<T>::value,
                  "matrix inversion requires a floating-point type");

    matrix<T, N, N> work = value;
    result = matrix<T, N, N>();

    for (size_type i = 0U; i < N; ++i)
    {
        result(i, i) = static_cast<T>(1);
    }

    for (size_type pivot = 0U; pivot < N; ++pivot)
    {
        size_type pivot_row = pivot;
        T pivot_abs = castle::math::abs(work(pivot, pivot));

        for (size_type row = pivot + 1U; row < N; ++row)
        {
            CASTLE_CONST T candidate = castle::math::abs(work(row, pivot));
            if (candidate > pivot_abs)
            {
                pivot_abs = candidate;
                pivot_row = row;
            }
        }

        if (pivot_abs <= epsilon)
        {
            result = matrix<T, N, N>();
            return false;
        }

        if (pivot_row != pivot)
        {
            for (size_type column = 0U; column < N; ++column)
            {
                CASTLE_CONST T temporary = work(pivot, column);
                work(pivot, column) = work(pivot_row, column);
                work(pivot_row, column) = temporary;

                CASTLE_CONST T inverse_temporary = result(pivot, column);
                result(pivot, column) = result(pivot_row, column);
                result(pivot_row, column) = inverse_temporary;
            }
        }

        CASTLE_CONST T inverse_pivot = static_cast<T>(1) / work(pivot, pivot);
        for (size_type column = 0U; column < N; ++column)
        {
            work(pivot, column) = static_cast<T>(work(pivot, column) * inverse_pivot);
            result(pivot, column) = static_cast<T>(result(pivot, column) * inverse_pivot);
        }

        for (size_type row = 0U; row < N; ++row)
        {
            if (row == pivot)
            {
                continue;
            }

            CASTLE_CONST T factor = work(row, pivot);
            if (castle::math::abs(factor) <= epsilon)
            {
                work(row, pivot) = static_cast<T>(0);
                continue;
            }

            for (size_type column = 0U; column < N; ++column)
            {
                work(row, column) = static_cast<T>(work(row, column) - factor * work(pivot, column));
                result(row, column) = static_cast<T>(result(row, column) - factor * result(pivot, column));
            }
        }
    }

    return true;
}

template <typename T, size_type N>
CASTLE_NODISCARD matrix<T, N, N> inverse(
    CASTLE_CONST matrix<T, N, N>& value,
    T epsilon = meta::floating_epsilon<T>::value) CASTLE_NOEXCEPT
{
    static_assert(meta::is_floating_point<T>::value,
                  "matrix inversion requires a floating-point type");

    matrix<T, N, N> result;
    CASTLE_ASSERT(try_inverse(value, result, epsilon),
                  CASTLE_ERROR_GENERIC("castle::math::inverse: singular matrix"));
    return result;
}

template <typename T, size_type N>
CASTLE_CONSTEXPR matrix<T, N, N> identity() CASTLE_NOEXCEPT
{
    matrix<T, N, N> result;
    for (size_type i = 0U; i < N; ++i)
    {
        result(i, i) = static_cast<T>(1);
    }
    return result;
}

template <typename T, size_type Rows, size_type Columns>
CASTLE_CONSTEXPR bool near_equal(
    CASTLE_CONST matrix<T, Rows, Columns>& first,
    CASTLE_CONST matrix<T, Rows, Columns>& second,
    T epsilon) CASTLE_NOEXCEPT
{
    static_assert(meta::is_floating_point<T>::value,
                  "matrix near_equal requires floating-point elements");

    for (size_type row = 0U; row < Rows; ++row)
    {
        for (size_type column = 0U; column < Columns; ++column)
        {
            if (!castle::math::near_equal(first(row, column), second(row, column), epsilon))
            {
                return false;
            }
        }
    }
    return true;
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_MATRIX_H
