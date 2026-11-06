// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file update_set.hpp
 * @brief Fixed-size set of "which data types changed", keyed by a compile-time type list.
 *
 * An update set is the notification payload of the data layer. Instead of copying data
 * structures through every layer, producers publish values into a `data_pool` and only tell
 * consumers which types changed; consumers then read just those types. Each type of the list
 * owns one bit, so the set is a plain value: cheap to copy, compare, merge, and filter.
 *
 * @code
 * struct imu {};
 * struct gnss {};
 *
 * using updates = castle_ext::data::update_set<imu, gnss>;
 *
 * updates changed = updates::of<imu>();
 * changed.insert<gnss>();
 * bool has_imu = changed.contains<imu>();   // true
 * changed.for_each([](auto type)
 * {
 *     using changed_type = typename decltype(type)::type;   // imu, then gnss
 * });
 * @endcode
 */
#ifndef CASTLE_EXT_DATA_UPDATE_SET_HPP
#define CASTLE_EXT_DATA_UPDATE_SET_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/utility/bitset.hpp"
#include "castle/utility/tuple.hpp"

namespace castle_ext
{
namespace data
{

template <typename... Ts>
class update_inbox;

/**
 * @brief Set of types, drawn from `Ts...`, that carry new data.
 *
 * @tparam Ts Unique data types. Each type maps to one bit, in declaration order.
 *
 * Every operation is O(number of storage words) or better and never allocates. Naming a type
 * that is not part of `Ts...` is a compile-time error. cv-qualifiers on the queried type are
 * ignored.
 */
template <typename... Ts>
class update_set
{
    static_assert(sizeof...(Ts) > 0U, "update_set requires at least one type");
    static_assert(castle::meta::has_unique_types<Ts...>::value, "update_set types must be unique");

    using bits_type = castle::bitset<sizeof...(Ts)>;

public:
    /** @brief Constructs an empty set. */
    update_set() CASTLE_NOEXCEPT CASTLE_DEFAULT;

    /**
     * @brief Returns a set that contains exactly `Us...`.
     * @tparam Us Types to include; each must be part of `Ts...`.
     */
    template <typename... Us>
    CASTLE_NODISCARD static update_set of() CASTLE_NOEXCEPT
    {
        update_set result;
        result.template insert<Us...>();
        return result;
    }

    /** @brief Returns a set that contains every type of `Ts...`. */
    CASTLE_NODISCARD static update_set all() CASTLE_NOEXCEPT
    {
        return of<Ts...>();
    }

    /** @brief Returns the number of types the set can describe, `sizeof...(Ts)`. */
    static CASTLE_CONSTEXPR castle::size_type capacity() CASTLE_NOEXCEPT
    {
        return sizeof...(Ts);
    }

    /** @brief Returns whether `T` is in the set. */
    template <typename T>
    CASTLE_NODISCARD bool contains() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return bits_.test(index_of<T>());
    }

    /** @brief Adds `Us...` to the set. */
    template <typename... Us>
    update_set& insert() CASTLE_NOEXCEPT
    {
        (bits_.set(index_of<Us>()), ...);
        return *this;
    }

    /** @brief Removes `Us...` from the set. */
    template <typename... Us>
    update_set& erase() CASTLE_NOEXCEPT
    {
        (bits_.reset(index_of<Us>()), ...);
        return *this;
    }

    /** @brief Removes every type from the set. */
    void clear() CASTLE_NOEXCEPT
    {
        bits_.reset();
    }

    /** @brief Returns whether the set contains no type. */
    CASTLE_NODISCARD bool empty() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return bits_.none();
    }

    /** @brief Returns how many types are in the set. */
    CASTLE_NODISCARD castle::size_type size() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return bits_.count();
    }

    /** @brief Adds every type of `other` (set union). */
    update_set& operator|=(CASTLE_CONST update_set& other) CASTLE_NOEXCEPT
    {
        bits_ |= other.bits_;
        return *this;
    }

    /** @brief Keeps only the types also in `other` (set intersection). */
    update_set& operator&=(CASTLE_CONST update_set& other) CASTLE_NOEXCEPT
    {
        bits_ &= other.bits_;
        return *this;
    }

    /** @brief Returns the union of two sets. */
    CASTLE_NODISCARD friend update_set operator|(update_set lhs, CASTLE_CONST update_set& rhs) CASTLE_NOEXCEPT
    {
        return lhs |= rhs;
    }

    /** @brief Returns the intersection of two sets. */
    CASTLE_NODISCARD friend update_set operator&(update_set lhs, CASTLE_CONST update_set& rhs) CASTLE_NOEXCEPT
    {
        return lhs &= rhs;
    }

    /** @brief Returns whether both sets contain the same types. */
    CASTLE_NODISCARD friend bool operator==(CASTLE_CONST update_set& lhs, CASTLE_CONST update_set& rhs) CASTLE_NOEXCEPT
    {
        return lhs.bits_ == rhs.bits_;
    }

    /** @brief Returns whether the sets differ. */
    CASTLE_NODISCARD friend bool operator!=(CASTLE_CONST update_set& lhs, CASTLE_CONST update_set& rhs) CASTLE_NOEXCEPT
    {
        return !(lhs == rhs);
    }

    /**
     * @brief Calls `visitor(castle::type_identity<T>{})` for every `T` in the set.
     * @param visitor Callable accepting `castle::type_identity<T>` for each type of `Ts...`;
     *                a generic lambda `[](auto type) { using T = typename decltype(type)::type; }`
     *                is the usual form.
     * @note Types are visited in declaration order. Complexity: O(sizeof...(Ts)).
     */
    template <typename Visitor>
    void for_each(Visitor&& visitor) CASTLE_CONST
    {
        (visit<Ts>(visitor), ...);
    }

private:
    template <typename...>
    friend class update_inbox;

    template <typename T>
    static CASTLE_CONSTEXPR castle::size_type index_of() CASTLE_NOEXCEPT
    {
        using value_type = castle::meta::remove_cv_t<T>;
        static_assert(castle::meta::pack_contains<value_type, Ts...>::value,
                      "type is not part of this update_set");
        return castle::meta::tuple_type_index<value_type, Ts...>::value;
    }

    template <typename T, typename Visitor>
    void visit(Visitor& visitor) CASTLE_CONST
    {
        if (contains<T>())
        {
            visitor(castle::type_identity<T>{});
        }
    }

    bits_type bits_{};
};

} // namespace data
} // namespace castle_ext

#endif // CASTLE_EXT_DATA_UPDATE_SET_HPP
