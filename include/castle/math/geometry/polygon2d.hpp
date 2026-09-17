// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @brief Fixed-capacity polygon container for 2D points.
 *
 * Use polygon2d when polygon vertices must live inside the object with a known
 * maximum capacity. Storage is deterministic and allocation-free, which makes
 * the type suitable for embedded geometry pipelines and control loops.
 */
#ifndef CASTLE_MATH_GEOMETRY_POLYGON2D_HPP
#define CASTLE_MATH_GEOMETRY_POLYGON2D_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/math/geometry/detail.hpp"
#include "castle/container/array.hpp"
#include "castle/error/status.hpp"
#include "castle/math/geometry/point2d.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Stores up to Capacity 2D vertices in boundary order.
 *
 * @tparam T Coordinate type.
 * @tparam Capacity Maximum number of stored vertices.
 * @note The type does not enforce convexity, winding, or closure.
 */
template <typename T, size_type Capacity>
class polygon2d
{
public:
    using value_type = point2d<T>;
    using container_type = container::array<value_type, Capacity>;
    using iterator = typename container_type::iterator;
    using const_iterator = typename container_type::const_iterator;

    /** @brief Constructs an empty polygon. */
    CASTLE_CONSTEXPR polygon2d() CASTLE_NOEXCEPT : points_(), size_(0U) {}

    /**
     * @brief Returns the current number of vertices.
     * @return Stored vertex count.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR size_type size() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return size_;
    }

    /**
     * @brief Returns the maximum number of vertices.
     * @return Compile-time capacity.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR size_type capacity() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return Capacity;
    }

    /**
     * @brief Reports whether the polygon stores no vertices.
     * @return true when size() is zero.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool empty() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return size_ == 0U;
    }

    /**
     * @brief Reports whether the polygon reached its fixed capacity.
     * @return true when size() equals Capacity.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool full() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return size_ == Capacity;
    }

    /**
     * @brief Computes twice the signed polygon area.
     * @return Positive for counter-clockwise winding, negative for clockwise winding, and zero for fewer than three vertices or zero area.
     * @note The computation uses geometry_calc_type<T> for the accumulator.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR typename detail::geometry_calc_type<T> signed_area2() CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (size_ < 3U)
        {
            return T{};
        }

        using calc_type = typename detail::geometry_calc_type<T>;
        calc_type area = calc_type{};
        size_type previous = size_ - 1U;
        for (size_type current = 0U; current < size_; ++current)
        {
            area = static_cast<calc_type>(area +
                                  static_cast<calc_type>(points_[previous].x()) * static_cast<calc_type>(points_[current].y()) -
                                  static_cast<calc_type>(points_[current].x()) * static_cast<calc_type>(points_[previous].y()));
            previous = current;
        }
        return area;
    }

    /**
     * @brief Reports whether the stored winding is clockwise.
     * @return true when signed_area2() is negative.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool clockwise() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return signed_area2() < T{};
    }

    /**
     * @brief Returns a read-only vertex reference.
     * @param index Vertex index in [0, size()).
     * @return Reference to the indexed vertex.
     * @warning index must be smaller than size().
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR CASTLE_CONST value_type& operator[](size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(index < size_, "polygon index out of range"); // LCOV_EXCL_BR_LINE
        return points_[index];
    }

    /**
     * @brief Returns a writable vertex reference.
     * @param index Vertex index in [0, size()).
     * @return Reference to the indexed vertex.
     * @warning index must be smaller than size().
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR value_type& operator[](size_type index) CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(index < size_, "polygon index out of range"); // LCOV_EXCL_BR_LINE
        return points_[index];
    }

    /**
     * @brief Returns an iterator to the first stored vertex.
     * @return Iterator pointing at the underlying storage start.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR iterator begin() CASTLE_NOEXCEPT
    {
        return points_.begin();
    }

    /**
     * @brief Returns a const iterator to the first stored vertex.
     * @return Const iterator pointing at the underlying storage start.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return points_.begin();
    }

    /**
     * @brief Returns an iterator one past the last stored vertex.
     * @return Iterator delimiting the active range.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR iterator end() CASTLE_NOEXCEPT
    {
        return points_.begin() + size_;
    }

    /**
     * @brief Returns a const iterator one past the last stored vertex.
     * @return Const iterator delimiting the active range.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return points_.begin() + size_;
    }

    /**
     * @brief Returns the underlying contiguous storage pointer.
     * @return Pointer to the first element of the embedded array.
     * @note Only the first size() entries are logically part of the polygon.
     */
    CASTLE_NODISCARD value_type* data() CASTLE_NOEXCEPT
    {
        return points_.data();
    }

    /**
     * @brief Returns the underlying contiguous storage pointer.
     * @return Const pointer to the first element of the embedded array.
     * @note Only the first size() entries are logically part of the polygon.
     */
    CASTLE_NODISCARD CASTLE_CONST value_type* data() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return points_.data();
    }

    /**
     * @brief Appends a vertex when capacity permits.
     * @param value Vertex to append.
     * @return status::ok on success or status::full when the polygon is full.
     */
    CASTLE_CONSTEXPR status push_back(CASTLE_CONST value_type& value) CASTLE_NOEXCEPT
    {
        if (size_ >= Capacity)
        {
            return status::full;
        }
        points_[size_] = value;
        ++size_;
        return status::ok;
    }

    /**
     * @brief Removes the last vertex.
     * @return status::ok on success or status::empty when the polygon is empty.
     */
    CASTLE_CONSTEXPR status pop_back() CASTLE_NOEXCEPT
    {
        if (size_ == 0U)
        {
            return status::empty;
        }
        --size_;
        return status::ok;
    }

    /** @brief Removes all stored vertices. */
    CASTLE_CONSTEXPR void clear() CASTLE_NOEXCEPT
    {
        size_ = 0U;
    }

private:
    container_type points_;
    size_type size_;
};

}
}

#endif
