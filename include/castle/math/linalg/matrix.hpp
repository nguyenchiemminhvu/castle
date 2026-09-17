// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file
 * @brief Fixed-size row-major matrices for Castle linear algebra.
 *
 * This header provides a compile-time-sized matrix type and supporting linear
 * algebra utilities for deterministic embedded code. Use it when matrix
 * dimensions are known at compile time and you need inline storage, no heap
 * allocation, no exceptions, no RTTI, no virtual dispatch, and no STL
 * dependency. Elements are stored contiguously in row-major order, while
 * multiplication follows the conventional `matrix * column_vector` model.
 * Determinants are computed recursively for small fixed matrices, and
 * floating-point inversion uses Gauss-Jordan elimination with partial
 * pivoting and an explicit singularity tolerance.
 *
 * @code
 * #include "castle/math/linalg/matrix.hpp"
 *
 * const castle::math::matrix<float, 2U, 2U> value(1.0F, 2.0F, 3.0F, 4.0F);
 * const castle::math::vector<float, 2U> input(1.0F, 0.0F);
 * const castle::math::vector<float, 2U> result = value * input;
 * @endcode
 */
#ifndef CASTLE_MATH_MATRIX_HPP
#define CASTLE_MATH_MATRIX_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/math/abs.hpp"
#include "castle/math/near_equal.hpp"
#include "castle/math/linalg/vector.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Fixed-size row-major matrix with inline contiguous storage.
 * @tparam T Arithmetic element type stored in the matrix.
 * @tparam Rows Compile-time number of rows.
 * @tparam Columns Compile-time number of columns.
 * @note Storage order is row-major: element `(r, c)` resides at
 * `data_[r * Columns + c]`.
 * @warning `operator()` does not perform bounds checking; use `at()` when the
 * caller cannot guarantee valid indices.
 */
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

    /** @brief Compile-time row count exposed as a constant. */
    static CASTLE_CONSTEXPR size_type static_rows = Rows;
    /** @brief Compile-time column count exposed as a constant. */
    static CASTLE_CONSTEXPR size_type static_columns = Columns;

    /**
     * @brief Constructs a zero-initialized matrix.
     * @return Matrix whose elements are value-initialized.
     * @note All storage lives inside the object with no allocation.
     */
    CASTLE_CONSTEXPR matrix() CASTLE_NOEXCEPT : data_{}
    {
    }

    /**
     * @brief Constructs a matrix from exactly `Rows * Columns` element values.
     * @tparam Args Element argument types convertible to `T`.
     * @param args Elements supplied in row-major order.
     * @return Matrix initialized from the supplied values.
     * @warning The constructor participates in overload resolution only when
     * exactly `Rows * Columns` arguments are provided.
     */
    template <typename... Args,
              meta::enable_if_t<(sizeof...(Args) == Rows * Columns) &&
                                meta::conjunction<meta::is_constructible<T, Args&&>...>::value, int> = 0>
    CASTLE_CONSTEXPR explicit matrix(Args&&... args)
        : data_{static_cast<T>(args)...}
    {
    }

    /**
     * @brief Returns the number of rows.
     * @return The compile-time row count `Rows`.
     * @note This operation is constant-time and `constexpr`.
     */
    CASTLE_CONSTEXPR size_type rows() CASTLE_CONST CASTLE_NOEXCEPT { return Rows; }

    /**
     * @brief Returns the number of columns.
     * @return The compile-time column count `Columns`.
     * @note This operation is constant-time and `constexpr`.
     */
    CASTLE_CONSTEXPR size_type columns() CASTLE_CONST CASTLE_NOEXCEPT { return Columns; }

    /**
     * @brief Returns the number of stored elements.
     * @return `Rows * Columns`.
     * @note This operation is constant-time and `constexpr`.
     */
    CASTLE_CONSTEXPR size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return Rows * Columns; }

    /**
     * @brief Returns an iterator to the first element.
     * @return Pointer to the first stored element.
     * @note Iteration follows row-major storage order.
     */
    CASTLE_CONSTEXPR iterator begin() CASTLE_NOEXCEPT { return data_; }

    /**
     * @brief Returns a const iterator to the first element.
     * @return Const pointer to the first stored element.
     * @note Iteration follows row-major storage order.
     */
    CASTLE_CONSTEXPR const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }

    /**
     * @brief Returns a const iterator to the first element.
     * @return Const pointer to the first stored element.
     * @note Equivalent to `begin()` on a const matrix.
     */
    CASTLE_CONSTEXPR const_iterator cbegin() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }

    /**
     * @brief Returns an iterator one past the final stored element.
     * @return Pointer one past the row-major storage range.
     * @note Iteration follows row-major storage order.
     */
    CASTLE_CONSTEXPR iterator end() CASTLE_NOEXCEPT { return data_ + Rows * Columns; }

    /**
     * @brief Returns a const iterator one past the final stored element.
     * @return Const pointer one past the row-major storage range.
     * @note Iteration follows row-major storage order.
     */
    CASTLE_CONSTEXPR const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return data_ + Rows * Columns; }

    /**
     * @brief Returns a const iterator one past the final stored element.
     * @return Const pointer one past the row-major storage range.
     * @note Equivalent to `end()` on a const matrix.
     */
    CASTLE_CONSTEXPR const_iterator cend() CASTLE_CONST CASTLE_NOEXCEPT { return data_ + Rows * Columns; }

    /**
     * @brief Returns the element at `(row, column)` without bounds checking.
     * @param row Zero-based row index.
     * @param column Zero-based column index.
     * @return Mutable reference to the selected element.
     * @warning Passing an out-of-range index is undefined for this API.
     */
    CASTLE_CONSTEXPR reference operator()(size_type row, size_type column) CASTLE_NOEXCEPT
    {
        return data_[row * Columns + column];
    }

    /**
     * @brief Returns the element at `(row, column)` without bounds checking.
     * @param row Zero-based row index.
     * @param column Zero-based column index.
     * @return Const reference to the selected element.
     * @warning Passing an out-of-range index is undefined for this API.
     */
    CASTLE_CONSTEXPR const_reference operator()(size_type row, size_type column) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return data_[row * Columns + column];
    }

    /**
     * @brief Returns the element at `(row, column)` with Castle assertion checking.
     * @param row Zero-based row index.
     * @param column Zero-based column index.
     * @return Mutable reference to the selected element.
     * @warning Triggers Castle's assertion/error path when the index is out of range.
     */
    CASTLE_CONSTEXPR reference at(size_type row, size_type column) CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(row < Rows && column < Columns, // LCOV_EXCL_BR_LINE
                      CASTLE_ERROR_GENERIC("castle::math::matrix::at: index out of range"));
        return data_[row * Columns + column];
    }

    /**
     * @brief Returns the element at `(row, column)` with Castle assertion checking.
     * @param row Zero-based row index.
     * @param column Zero-based column index.
     * @return Const reference to the selected element.
     * @warning Triggers Castle's assertion/error path when the index is out of range.
     */
    CASTLE_CONSTEXPR const_reference at(size_type row, size_type column) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(row < Rows && column < Columns, // LCOV_EXCL_BR_LINE
                      CASTLE_ERROR_GENERIC("castle::math::matrix::at: index out of range"));
        return data_[row * Columns + column];
    }

    /**
     * @brief Returns the row-major storage pointer.
     * @return Pointer to the first stored element.
     * @note The returned pointer remains valid for the lifetime of the matrix.
     */
    CASTLE_CONSTEXPR pointer data() CASTLE_NOEXCEPT { return data_; }

    /**
     * @brief Returns the row-major storage pointer.
     * @return Const pointer to the first stored element.
     * @note The returned pointer remains valid for the lifetime of the matrix.
     */
    CASTLE_CONSTEXPR const_pointer data() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }

    /**
     * @brief Returns an unchanged copy of the matrix.
     * @return Copy of `*this`.
     * @note Provided for arithmetic symmetry with unary minus.
     */
    CASTLE_CONSTEXPR matrix operator+() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return *this;
    }

    /**
     * @brief Returns the element-wise negation of the matrix.
     * @return Matrix whose elements are `-data_[i]`.
     * @note Elements are negated in row-major storage order.
     */
    CASTLE_CONSTEXPR matrix operator-() CASTLE_CONST CASTLE_NOEXCEPT
    {
        matrix result;
        for (size_type i = 0U; i < Rows * Columns; ++i)
        {
            result.data_[i] = static_cast<T>(-data_[i]);
        }
        return result;
    }

    /**
     * @brief Adds two matrices element by element.
     * @param other Matrix added to `*this`.
     * @return Element-wise sum.
     * @note Both matrices have the same dimensions and element type.
     */
    CASTLE_CONSTEXPR matrix operator+(CASTLE_CONST matrix& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        matrix result;
        for (size_type i = 0U; i < Rows * Columns; ++i)
        {
            result.data_[i] = static_cast<T>(data_[i] + other.data_[i]);
        }
        return result;
    }

    /**
     * @brief Subtracts two matrices element by element.
     * @param other Matrix subtracted from `*this`.
     * @return Element-wise difference.
     * @note Both matrices have the same dimensions and element type.
     */
    CASTLE_CONSTEXPR matrix operator-(CASTLE_CONST matrix& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        matrix result;
        for (size_type i = 0U; i < Rows * Columns; ++i)
        {
            result.data_[i] = static_cast<T>(data_[i] - other.data_[i]);
        }
        return result;
    }

    /**
     * @brief Adds another matrix into this matrix element by element.
     * @param other Matrix added to `*this`.
     * @return Reference to `*this`.
     * @note The update is performed in place with no temporary allocation.
     */
    CASTLE_CONSTEXPR matrix& operator+=(CASTLE_CONST matrix& other) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < Rows * Columns; ++i)
        {
            data_[i] = static_cast<T>(data_[i] + other.data_[i]);
        }
        return *this;
    }

    /**
     * @brief Subtracts another matrix from this matrix element by element.
     * @param other Matrix subtracted from `*this`.
     * @return Reference to `*this`.
     * @note The update is performed in place with no temporary allocation.
     */
    CASTLE_CONSTEXPR matrix& operator-=(CASTLE_CONST matrix& other) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < Rows * Columns; ++i)
        {
            data_[i] = static_cast<T>(data_[i] - other.data_[i]);
        }
        return *this;
    }

    /**
     * @brief Multiplies every element by `scalar`.
     * @param scalar Scalar multiplier.
     * @return Scaled matrix.
     * @note Arithmetic follows the rules of `T`.
     */
    CASTLE_CONSTEXPR matrix operator*(T scalar) CASTLE_CONST CASTLE_NOEXCEPT
    {
        matrix result;
        for (size_type i = 0U; i < Rows * Columns; ++i)
        {
            result.data_[i] = static_cast<T>(data_[i] * scalar);
        }
        return result;
    }

    /**
     * @brief Divides every element by `scalar`.
     * @param scalar Scalar divisor.
     * @return Scaled matrix.
     * @warning Triggers Castle's assertion/error path when `scalar == 0`.
     */
    matrix operator/(T scalar) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(scalar != static_cast<T>(0), // LCOV_EXCL_BR_LINE
                      CASTLE_ERROR_GENERIC("castle::math::matrix: division by zero"));

        matrix result;
        for (size_type i = 0U; i < Rows * Columns; ++i)
        {
            result.data_[i] = static_cast<T>(data_[i] / scalar);
        }
        return result;
    }

    /**
     * @brief Multiplies every element by `scalar` in place.
     * @param scalar Scalar multiplier.
     * @return Reference to `*this`.
     * @note Arithmetic follows the rules of `T`.
     */
    CASTLE_CONSTEXPR matrix& operator*=(T scalar) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < Rows * Columns; ++i)
        {
            data_[i] = static_cast<T>(data_[i] * scalar);
        }
        return *this;
    }

    /**
     * @brief Divides every element by `scalar` in place.
     * @param scalar Scalar divisor.
     * @return Reference to `*this`.
     * @warning Triggers Castle's assertion/error path when `scalar == 0`.
     */
    matrix& operator/=(T scalar) CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(scalar != static_cast<T>(0), // LCOV_EXCL_BR_LINE
                      CASTLE_ERROR_GENERIC("castle::math::matrix: division by zero"));

        for (size_type i = 0U; i < Rows * Columns; ++i)
        {
            data_[i] = static_cast<T>(data_[i] / scalar);
        }
        return *this;
    }

    /**
     * @brief Compares two matrices element by element for exact equality.
     * @param other Matrix compared with `*this`.
     * @return `true` when every element is exactly equal.
     * @warning For floating-point matrices, use `near_equal()` when a tolerance is required.
     */
    CASTLE_CONSTEXPR bool operator==(CASTLE_CONST matrix& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < Rows * Columns; ++i)
        {
            // LCOV_EXCL_START
            if (data_[i] != other.data_[i])
            {
                return false;
            }
            // LCOV_EXCL_STOP
        }
        return true;
    }

    /**
     * @brief Compares two matrices element by element for exact inequality.
     * @param other Matrix compared with `*this`.
     * @return `true` when any element differs exactly.
     * @warning For floating-point matrices, use `near_equal()` when a tolerance is required.
     */
    CASTLE_CONSTEXPR bool operator!=(CASTLE_CONST matrix& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return !(*this == other);
    }

    /**
     * @brief Extracts one row as a fixed-size vector.
     * @param index Zero-based row index.
     * @return Row `index` in left-to-right order.
     * @warning Triggers Castle's assertion/error path when `index >= Rows`.
     */
    CASTLE_CONSTEXPR vector<T, Columns> row(size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(index < Rows, // LCOV_EXCL_BR_LINE
                      CASTLE_ERROR_GENERIC("castle::math::matrix::row: index out of range"));
        vector<T, Columns> result;
        for (size_type i = 0U; i < Columns; ++i)
        {
            result[i] = data_[index * Columns + i];
        }
        return result;
    }

    /**
     * @brief Extracts one column as a fixed-size vector.
     * @param index Zero-based column index.
     * @return Column `index` in top-to-bottom order.
     * @warning Triggers Castle's assertion/error path when `index >= Columns`.
     */
    CASTLE_CONSTEXPR vector<T, Rows> column(size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(index < Columns, // LCOV_EXCL_BR_LINE
                      CASTLE_ERROR_GENERIC("castle::math::matrix::column: index out of range"));
        vector<T, Rows> result;
        for (size_type i = 0U; i < Rows; ++i)
        {
            result[i] = data_[i * Columns + index];
        }
        return result;
    }

    /**
     * @brief Returns the transpose of the matrix.
     * @return Matrix with rows and columns exchanged.
     * @note The returned matrix has dimensions `Columns x Rows`.
     */
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

    /**
     * @brief Returns the trace of a square matrix.
     * @return Sum of the diagonal elements.
     * @warning This member is available only when `Rows == Columns`.
     */
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

/**
 * @brief Multiplies a matrix by a scalar with the scalar on the left.
 * @tparam T Matrix element type.
 * @tparam Rows Compile-time number of rows.
 * @tparam Columns Compile-time number of columns.
 * @param scalar Scalar multiplier.
 * @param value Matrix operand.
 * @return `value * scalar`.
 */
template <typename T, size_type Rows, size_type Columns>
CASTLE_CONSTEXPR matrix<T, Rows, Columns> operator*(
    T scalar,
    CASTLE_CONST matrix<T, Rows, Columns>& value) CASTLE_NOEXCEPT
{
    return value * scalar;
}

/**
 * @brief Adds matrices element by element with common-type promotion.
 * @tparam T Left-hand element type.
 * @tparam Rows Compile-time number of rows.
 * @tparam Columns Compile-time number of columns.
 * @tparam U Right-hand element type.
 * @param first Left-hand matrix operand.
 * @param second Right-hand matrix operand.
 * @return Matrix of the common type containing the element-wise sum.
 */
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

/**
 * @brief Subtracts matrices element by element with common-type promotion.
 * @tparam T Left-hand element type.
 * @tparam Rows Compile-time number of rows.
 * @tparam Columns Compile-time number of columns.
 * @tparam U Right-hand element type.
 * @param first Left-hand matrix operand.
 * @param second Right-hand matrix operand.
 * @return Matrix of the common type containing the element-wise difference.
 */
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

/**
 * @brief Multiplies two matrices using the standard row-by-column product.
 * @tparam T Left-hand element type.
 * @tparam Rows Left-hand row count.
 * @tparam Columns Shared inner dimension.
 * @tparam U Right-hand element type.
 * @tparam OtherColumns Right-hand column count.
 * @param first Left-hand matrix operand.
 * @param second Right-hand matrix operand.
 * @return Matrix product with common-type element promotion.
 * @note Inputs are stored row-major, but the computed product still follows
 * the conventional algebraic definition.
 */
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
                // Accumulate the dot product of row r and column c.
                result(r, c) = static_cast<result_type>(
                    result(r, c) + static_cast<result_type>(first(r, k)) * static_cast<result_type>(second(k, c)));
            }
        }
    }
    return result;
}

/**
 * @brief Multiplies a matrix by a same-type column vector.
 * @tparam T Element type shared by the matrix and vector.
 * @tparam Rows Matrix row count and result dimension.
 * @tparam Columns Matrix column count and input dimension.
 * @param value Matrix operand.
 * @param input Column vector operand.
 * @return Matrix-vector product.
 * @note Vectors are treated as columns, so each output component is a row dot product.
 */
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

/**
 * @brief Multiplies a matrix by a column vector with common-type promotion.
 * @tparam T Matrix element type.
 * @tparam Rows Matrix row count and result dimension.
 * @tparam Columns Matrix column count and input dimension.
 * @tparam U Vector element type.
 * @param value Matrix operand.
 * @param input Column vector operand.
 * @return Matrix-vector product promoted to the common type.
 * @note Vectors are treated as columns, so each output component is a row dot product.
 */
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

/**
 * @brief Returns the transpose of a matrix.
 * @tparam T Matrix element type.
 * @tparam Rows Input row count.
 * @tparam Columns Input column count.
 * @param value Matrix to transpose.
 * @return Transposed matrix.
 * @note Equivalent to `value.transpose()`.
 */
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

/**
 * @brief Returns the Hadamard product of two matrices.
 * @tparam T Matrix element type.
 * @tparam Rows Compile-time number of rows.
 * @tparam Columns Compile-time number of columns.
 * @param first Left-hand matrix operand.
 * @param second Right-hand matrix operand.
 * @return Element-wise product matrix.
 */
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

/**
 * @brief Returns the squared Frobenius norm of a matrix.
 * @tparam T Matrix element type.
 * @tparam Rows Compile-time number of rows.
 * @tparam Columns Compile-time number of columns.
 * @param value Matrix whose squared Frobenius norm is computed.
 * @return Sum of squares of all matrix elements.
 * @note This avoids taking a square root and is available for any arithmetic `T`.
 */
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

/**
 * @brief Builds a square diagonal matrix from vector components.
 * @tparam T Matrix and vector element type.
 * @tparam N Matrix dimension.
 * @param values Diagonal element values.
 * @return `N x N` matrix with `values[i]` on the diagonal and zeros elsewhere.
 */
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

/**
 * @brief Builds the 3x3 skew-symmetric matrix for a 3D vector.
 * @tparam T Matrix and vector element type.
 * @param value Input vector.
 * @return Matrix `S` such that `S * other == cross(value, other)` for column vectors.
 * @note The returned matrix is stored row-major like every `castle::math::matrix`.
 */
template <typename T>
CASTLE_CONSTEXPR matrix<T, 3U, 3U> skew_symmetric(
    CASTLE_CONST vector<T, 3U>& value) CASTLE_NOEXCEPT
{
    return matrix<T, 3U, 3U>(
        static_cast<T>(0), static_cast<T>(-value[2U]), value[1U],
        value[2U], static_cast<T>(0), static_cast<T>(-value[0U]),
        static_cast<T>(-value[1U]), value[0U], static_cast<T>(0));
}

/**
 * @brief Returns the outer product of two vectors.
 * @tparam T Vector and matrix element type.
 * @tparam N Shared vector dimension and matrix dimension.
 * @param first Left-hand vector operand.
 * @param second Right-hand vector operand.
 * @return Square matrix with elements `first[row] * second[column]`.
 */
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

/** @brief Convenience alias for a 2x2 matrix. */
template <typename T>
using matrix2x2 = matrix<T, 2U, 2U>;

/** @brief Convenience alias for a 3x3 matrix. */
template <typename T>
using matrix3x3 = matrix<T, 3U, 3U>;

/** @brief Convenience alias for a 4x4 matrix. */
template <typename T>
using matrix4x4 = matrix<T, 4U, 4U>;

/**
 * @brief Recursive determinant implementation helper.
 * @tparam T Matrix element type.
 * @tparam N Square matrix dimension.
 * @note This type is exposed in the public header but is primarily an
 * implementation detail of `determinant()`.
 */
template <typename T, size_type N>
struct determinant_helper;

/**
 * @brief Determinant helper specialization for 1x1 matrices.
 * @tparam T Matrix element type.
 */
template <typename T>
struct determinant_helper<T, 1U>
{
    /**
     * @brief Returns the determinant of a 1x1 matrix.
     * @param value Matrix whose determinant is computed.
     * @return The sole stored element.
     */
    static CASTLE_CONSTEXPR T compute(CASTLE_CONST matrix<T, 1U, 1U>& value) CASTLE_NOEXCEPT
    {
        return value(0U, 0U);
    }
};

/**
 * @brief Determinant helper specialization for 2x2 matrices.
 * @tparam T Matrix element type.
 */
template <typename T>
struct determinant_helper<T, 2U>
{
    /**
     * @brief Returns the determinant of a 2x2 matrix.
     * @param value Matrix whose determinant is computed.
     * @return `a*d - b*c`.
     */
    static CASTLE_CONSTEXPR T compute(CASTLE_CONST matrix<T, 2U, 2U>& value) CASTLE_NOEXCEPT
    {
        return static_cast<T>(value(0U, 0U) * value(1U, 1U) -
                              value(0U, 1U) * value(1U, 0U));
    }
};

/**
 * @brief Determinant helper for larger square matrices.
 * @tparam T Matrix element type.
 * @tparam N Square matrix dimension.
 * @note Uses recursive cofactor expansion along the first row.
 */
template <typename T, size_type N>
struct determinant_helper
{
    /**
     * @brief Returns the determinant of an `N x N` matrix.
     * @param value Matrix whose determinant is computed.
     * @return Determinant value.
     * @warning Cost grows quickly with `N`; this helper is intended for small fixed matrices.
     */
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

/**
 * @brief Returns the determinant of a square matrix.
 * @tparam T Matrix element type.
 * @tparam N Square matrix dimension.
 * @param value Matrix whose determinant is computed.
 * @return Determinant value.
 * @warning Cost grows quickly with `N`; this function is intended for small fixed matrices.
 */
template <typename T, size_type N>
CASTLE_NODISCARD T determinant(CASTLE_CONST matrix<T, N, N>& value) CASTLE_NOEXCEPT
{
    return determinant_helper<T, N>::compute(value);
}

/**
 * @brief Attempts to invert a floating-point square matrix.
 * @tparam T Floating-point element type.
 * @tparam N Square matrix dimension.
 * @param value Matrix to invert.
 * @param result Output matrix that receives the inverse on success or zeros on failure.
 * @param epsilon Singularity tolerance used for pivot testing.
 * @return `true` when the matrix is invertible within the supplied tolerance.
 * @note The implementation uses Gauss-Jordan elimination with partial pivoting.
 * @warning Available only for floating-point element types.
 */
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
            CASTLE_CONST T candidate = castle::math::abs(work(row, pivot)); // LCOV_EXCL_BR_LINE
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

/**
 * @brief Inverts a floating-point square matrix and asserts on failure.
 * @tparam T Floating-point element type.
 * @tparam N Square matrix dimension.
 * @param value Matrix to invert.
 * @param epsilon Singularity tolerance used for pivot testing.
 * @return Inverse matrix.
 * @warning Triggers Castle's assertion/error path when the matrix is singular
 * within the supplied tolerance and is available only for floating-point types.
 */
template <typename T, size_type N>
CASTLE_NODISCARD matrix<T, N, N> inverse(
    CASTLE_CONST matrix<T, N, N>& value,
    T epsilon = meta::floating_epsilon<T>::value) CASTLE_NOEXCEPT
{
    static_assert(meta::is_floating_point<T>::value,
                  "matrix inversion requires a floating-point type");

    matrix<T, N, N> result;
    CASTLE_ASSERT(try_inverse(value, result, epsilon), // LCOV_EXCL_BR_LINE
                  CASTLE_ERROR_GENERIC("castle::math::inverse: singular matrix"));
    return result;
}

/**
 * @brief Builds an identity matrix.
 * @tparam T Matrix element type.
 * @tparam N Square matrix dimension.
 * @return `N x N` identity matrix.
 * @note The returned matrix is stored row-major and acts as the multiplicative identity.
 */
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

/**
 * @brief Compares two floating-point matrices with a relative tolerance.
 * @tparam T Floating-point element type.
 * @tparam Rows Compile-time number of rows.
 * @tparam Columns Compile-time number of columns.
 * @param first Left-hand matrix operand.
 * @param second Right-hand matrix operand.
 * @param epsilon Relative tolerance forwarded to scalar `near_equal()`.
 * @return `true` when every element compares equal within tolerance.
 * @warning This helper is available only for floating-point element types.
 */
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

#endif
