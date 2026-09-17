#ifndef CASTLE_MATH_GEOMETRY_POLYGON2D_H
#define CASTLE_MATH_GEOMETRY_POLYGON2D_H

#include "castle/core/compiler.h"
#include "castle/core/error_handler.h"
#include "castle/core/traits.h"
#include "castle/core/types.h"
#include "castle/math/geometry/detail.h"
#include "castle/container/array.h"
#include "castle/error/status.h"
#include "castle/math/geometry/point2d.h"

namespace castle
{
namespace math
{

// ============================================================================
// Fixed-capacity polygon of 2D points.
//
// Storage is embedded in the object. There is no heap allocation and all
// traversal is deterministic. The points are expected to be in boundary order.
//
// Real-life use cases: forbidden zones, room/map outlines, collision masks,
// and field-of-view polygons in embedded robotics.
// ============================================================================
template <typename T, size_type Capacity>
class polygon2d
{
public:
    using value_type = point2d<T>;
    using container_type = container::array<value_type, Capacity>;
    using iterator = typename container_type::iterator;
    using const_iterator = typename container_type::const_iterator;

    CASTLE_CONSTEXPR polygon2d() CASTLE_NOEXCEPT : points_(), size_(0U) {}

    CASTLE_NODISCARD CASTLE_CONSTEXPR size_type size() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return size_;
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR size_type capacity() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return Capacity;
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR bool empty() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return size_ == 0U;
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR bool full() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return size_ == Capacity;
    }

    // ------------------------------------------------------------------------
    // Twice the signed area. Positive means counter-clockwise vertex order.
    // O(N) time, O(1) storage. Keeping twice-area avoids an unnecessary divide.
    // ------------------------------------------------------------------------
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

    CASTLE_NODISCARD CASTLE_CONSTEXPR bool clockwise() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return signed_area2() < T{};
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR CASTLE_CONST value_type& operator[](size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(index < size_, "polygon index out of range");
        return points_[index];
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR value_type& operator[](size_type index) CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(index < size_, "polygon index out of range");
        return points_[index];
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR iterator begin() CASTLE_NOEXCEPT
    {
        return points_.begin();
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return points_.begin();
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR iterator end() CASTLE_NOEXCEPT
    {
        return points_.begin() + size_;
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return points_.begin() + size_;
    }

    CASTLE_NODISCARD value_type* data() CASTLE_NOEXCEPT
    {
        return points_.data();
    }

    CASTLE_NODISCARD CASTLE_CONST value_type* data() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return points_.data();
    }

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

    CASTLE_CONSTEXPR status pop_back() CASTLE_NOEXCEPT
    {
        if (size_ == 0U)
        {
            return status::empty;
        }
        --size_;
        return status::ok;
    }

    CASTLE_CONSTEXPR void clear() CASTLE_NOEXCEPT
    {
        size_ = 0U;
    }

private:
    container_type points_;
    size_type size_;
};

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_GEOMETRY_POLYGON2D_H
