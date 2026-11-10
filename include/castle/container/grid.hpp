// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file grid.hpp
 * @brief Fixed-size two-dimensional container with inline row-major storage.
 *
 * Use `grid<T, Rows, Columns>` when a table of elements has dimensions known
 * at compile time: lookup tables, calibration maps, occupancy maps, tile maps,
 * or the backing storage of higher-level types such as
 * `castle::math::matrix`. All `Rows * Columns` elements live directly inside
 * the object in row-major order, so the container never allocates and every
 * access is O(1). The grid is purely a data structure; it defines no
 * arithmetic.
 *
 * @code
 * castle::container::grid<uint8_t, 2U, 3U> map{0U, 1U, 2U,
 *                                              3U, 4U, 5U};
 * map(1U, 2U) = 50U;
 * castle::container::array_view<uint8_t> second_row = map.row(1U);
 * @endcode
 */
#ifndef CASTLE_CONTAINER_GRID_HPP
#define CASTLE_CONTAINER_GRID_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/container/array_view.hpp"
#include "castle/container/initializer_list.hpp"
#include "castle/iterator/reverse_iterator.hpp"
#include "castle/utility/forward.hpp"

namespace castle
{
namespace container
{

/**
 * @brief Fixed-size two-dimensional container with contiguous row-major storage.
 * @tparam T Element type. Must be default-constructible.
 * @tparam Rows Compile-time number of rows. Must be greater than zero.
 * @tparam Columns Compile-time number of columns. Must be greater than zero.
 * @note Element `(r, c)` resides at `data()[r * Columns + c]`. Iteration
 * visits elements in row-major order.
 * @note Iterators, pointers, references, and row views remain valid until the
 * grid object itself is destroyed.
 * @warning `operator()` does not perform bounds checking; use `at()` when the
 * caller cannot guarantee valid indices.
 */
template <typename T, size_type Rows, size_type Columns>
class grid
{
    static_assert(Rows > 0U, "container::grid requires a positive row count");
    static_assert(Columns > 0U, "container::grid requires a positive column count");

public:
    using value_type = T;
    using size_type = castle::size_type;
    using difference_type = castle::difference_type;
    using reference = T&;
    using const_reference = CASTLE_CONST T&;
    using pointer = T*;
    using const_pointer = CASTLE_CONST T*;
    using iterator = T*;
    using const_iterator = CASTLE_CONST T*;
    using reverse_iterator = castle::reverse_iterator<iterator>;
    using const_reverse_iterator = castle::reverse_iterator<const_iterator>;
    using row_view = array_view<T>;
    using const_row_view = array_view<CASTLE_CONST T>;

    /** @brief Compile-time row count. */
    static CASTLE_CONSTEXPR size_type static_rows = Rows;
    /** @brief Compile-time column count. */
    static CASTLE_CONSTEXPR size_type static_columns = Columns;
    /** @brief Compile-time element count, `Rows * Columns`. */
    static CASTLE_CONSTEXPR size_type static_size = Rows * Columns;

    /**
     * @brief Value-initializes every element.
     * @note Arithmetic elements become zero. Complexity: O(Rows * Columns).
     */
    CASTLE_CONSTEXPR grid() CASTLE_NOEXCEPT : data_{}
    {
    }

    /**
     * @brief Constructs the grid from exactly `Rows * Columns` values.
     * @tparam Args Argument types explicitly convertible to `T`.
     * @param args Element values supplied in row-major order.
     * @note Each argument is converted with `static_cast<T>`. Complexity: O(Rows * Columns).
     * @warning Participates in overload resolution only when exactly
     * `Rows * Columns` arguments are supplied.
     */
    template <typename... Args,
              meta::enable_if_t<(sizeof...(Args) == Rows * Columns) &&
                                meta::conjunction<meta::is_constructible<T, Args&&>...>::value, int> = 0>
    CASTLE_CONSTEXPR explicit grid(Args&&... args)
        : data_{static_cast<T>(CASTLE_FORWARD<Args>(args))...}
    {
    }

    /**
     * @brief Constructs the grid from a row-major initializer list.
     * @param init Initializer list supplying exactly `Rows * Columns` elements.
     * @note Complexity: O(Rows * Columns).
     * @warning `init.size()` must equal `Rows * Columns`; a mismatch triggers
     * Castle's assertion/error path.
     */
    CASTLE_CONSTEXPR grid(initializer_list<T> init) : data_{}
    {
        CASTLE_ASSERT(init.size() == Rows * Columns, // LCOV_EXCL_BR_LINE
                      CASTLE_ERROR_GENERIC("castle::container::grid: initializer list size mismatch"));

        size_type i = 0U;
        for (CASTLE_CONST T& element : init)
        {
            data_[i] = element;
            ++i;
        }
    }

    /**
     * @brief Returns the number of rows.
     * @return Always `Rows`.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR size_type rows() CASTLE_CONST CASTLE_NOEXCEPT { return Rows; }

    /**
     * @brief Returns the number of columns.
     * @return Always `Columns`.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR size_type columns() CASTLE_CONST CASTLE_NOEXCEPT { return Columns; }

    /**
     * @brief Returns the number of stored elements.
     * @return Always `Rows * Columns`.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return Rows * Columns; }

    /**
     * @brief Reports whether the grid has no elements.
     * @return Always `false`, because both dimensions are positive.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return false; }

    /** @brief Returns an iterator to the first element in row-major order. */
    CASTLE_CONSTEXPR iterator begin() CASTLE_NOEXCEPT { return data_; }

    /** @brief Returns a const iterator to the first element in row-major order. */
    CASTLE_CONSTEXPR const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }

    /** @brief Returns a const iterator to the first element in row-major order. */
    CASTLE_CONSTEXPR const_iterator cbegin() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }

    /** @brief Returns an iterator one past the last element. */
    CASTLE_CONSTEXPR iterator end() CASTLE_NOEXCEPT { return data_ + Rows * Columns; }

    /** @brief Returns a const iterator one past the last element. */
    CASTLE_CONSTEXPR const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT { return data_ + Rows * Columns; }

    /** @brief Returns a const iterator one past the last element. */
    CASTLE_CONSTEXPR const_iterator cend() CASTLE_CONST CASTLE_NOEXCEPT { return data_ + Rows * Columns; }

    /** @brief Returns a reverse iterator to the last element. */
    CASTLE_CONSTEXPR reverse_iterator rbegin() CASTLE_NOEXCEPT { return reverse_iterator(end()); }

    /** @brief Returns a const reverse iterator to the last element. */
    CASTLE_CONSTEXPR const_reverse_iterator rbegin() CASTLE_CONST CASTLE_NOEXCEPT { return const_reverse_iterator(end()); }

    /** @brief Returns a const reverse iterator to the last element. */
    CASTLE_CONSTEXPR const_reverse_iterator crbegin() CASTLE_CONST CASTLE_NOEXCEPT { return const_reverse_iterator(end()); }

    /** @brief Returns a reverse iterator one before the first element. */
    CASTLE_CONSTEXPR reverse_iterator rend() CASTLE_NOEXCEPT { return reverse_iterator(begin()); }

    /** @brief Returns a const reverse iterator one before the first element. */
    CASTLE_CONSTEXPR const_reverse_iterator rend() CASTLE_CONST CASTLE_NOEXCEPT { return const_reverse_iterator(begin()); }

    /** @brief Returns a const reverse iterator one before the first element. */
    CASTLE_CONSTEXPR const_reverse_iterator crend() CASTLE_CONST CASTLE_NOEXCEPT { return const_reverse_iterator(begin()); }

    /**
     * @brief Returns the element at `(row, column)` without bounds checking.
     * @param row Zero-based row index.
     * @param column Zero-based column index.
     * @return Mutable reference to the selected element.
     * @warning `row < Rows` and `column < Columns` are preconditions.
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
     * @warning `row < Rows` and `column < Columns` are preconditions.
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
     * @warning Triggers Castle's assertion/error path when an index is out of range.
     */
    CASTLE_CONSTEXPR reference at(size_type row, size_type column) CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(row < Rows && column < Columns, // LCOV_EXCL_BR_LINE
                      CASTLE_ERROR_GENERIC("castle::container::grid::at: index out of range"));
        return data_[row * Columns + column];
    }

    /**
     * @brief Returns the element at `(row, column)` with Castle assertion checking.
     * @param row Zero-based row index.
     * @param column Zero-based column index.
     * @return Const reference to the selected element.
     * @warning Triggers Castle's assertion/error path when an index is out of range.
     */
    CASTLE_CONSTEXPR const_reference at(size_type row, size_type column) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(row < Rows && column < Columns, // LCOV_EXCL_BR_LINE
                      CASTLE_ERROR_GENERIC("castle::container::grid::at: index out of range"));
        return data_[row * Columns + column];
    }

    /**
     * @brief Returns a mutable view over one contiguous row.
     * @param index Zero-based row index.
     * @return View of `Columns` elements starting at `(index, 0)`.
     * @note Complexity: O(1). The view aliases the grid storage.
     * @warning Triggers Castle's assertion/error path when `index >= Rows`.
     */
    row_view row(size_type index) CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(index < Rows, // LCOV_EXCL_BR_LINE
                      CASTLE_ERROR_GENERIC("castle::container::grid::row: index out of range"));
        return row_view(data_ + index * Columns, Columns);
    }

    /**
     * @brief Returns a read-only view over one contiguous row.
     * @param index Zero-based row index.
     * @return View of `Columns` elements starting at `(index, 0)`.
     * @note Complexity: O(1). The view aliases the grid storage.
     * @warning Triggers Castle's assertion/error path when `index >= Rows`.
     */
    const_row_view row(size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(index < Rows, // LCOV_EXCL_BR_LINE
                      CASTLE_ERROR_GENERIC("castle::container::grid::row: index out of range"));
        return const_row_view(data_ + index * Columns, Columns);
    }

    /**
     * @brief Returns the row-major storage pointer.
     * @return Pointer to the first stored element.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR pointer data() CASTLE_NOEXCEPT { return data_; }

    /**
     * @brief Returns the row-major storage pointer.
     * @return Const pointer to the first stored element.
     * @note Complexity: O(1).
     */
    CASTLE_CONSTEXPR const_pointer data() CASTLE_CONST CASTLE_NOEXCEPT { return data_; }

    /**
     * @brief Assigns `value` to every element.
     * @param value Value copied into each cell.
     * @note Complexity: O(Rows * Columns).
     */
    CASTLE_CONSTEXPR void fill(CASTLE_CONST T& value) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < Rows * Columns; ++i)
        {
            data_[i] = value;
        }
    }

    /**
     * @brief Compares two grids element by element for exact equality.
     * @param other Grid compared with `*this`.
     * @return `true` when every element compares equal with `operator==`.
     * @note Complexity: O(Rows * Columns); stops at the first mismatch.
     */
    CASTLE_CONSTEXPR bool operator==(CASTLE_CONST grid& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < Rows * Columns; ++i)
        {
            if (!(data_[i] == other.data_[i]))
            {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief Compares two grids element by element for inequality.
     * @param other Grid compared with `*this`.
     * @return `true` when any element differs.
     * @note Complexity: O(Rows * Columns).
     */
    CASTLE_CONSTEXPR bool operator!=(CASTLE_CONST grid& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return !(*this == other);
    }

private:
    T data_[Rows * Columns];
};

}
}

#endif
