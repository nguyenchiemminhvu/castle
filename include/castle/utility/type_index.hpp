// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file type_index.hpp
 * @brief Copyable, ordered, hashable handle to a `castle::type_info`, like `std::type_index`.
 *
 * Use this header when type identities must be stored as values: keys of fixed-capacity maps
 * and hash tables, members of registries, or entries in sorted tables. A `type_index` is one
 * pointer, trivially copyable, and never allocates or throws. `castle::hash<type_index>` and
 * the relational operators make it usable with Castle's associative containers directly.
 *
 * @code
 * castle::type_index key = castle::type_id<uint32_t>();
 * castle::size_type bucket = castle::hash<castle::type_index>()(key);
 * @endcode
 */
#ifndef CASTLE_UTILITY_TYPE_INDEX_HPP
#define CASTLE_UTILITY_TYPE_INDEX_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/utility/hash.hpp"
#include "castle/utility/type_info.hpp"

namespace castle
{

/**
 * @brief Value wrapper around a `type_info` reference.
 * @note Equality, ordering, hashing, and naming all forward to the wrapped `type_info`.
 */
class type_index
{
public:
    /** @brief Wraps `info`; implicit so `type_id<T>()` converts where a `type_index` is expected. */
    CASTLE_CONSTEXPR type_index(CASTLE_CONST type_info& info) CASTLE_NOEXCEPT
        : info_(&info)
    {
    }

    /** @brief Returns the wrapped type's name; see `type_info::name()`. */
    CASTLE_NODISCARD CASTLE_CONST char* name() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return info_->name();
    }

    /** @brief Returns the wrapped type's hash; see `type_info::hash_code()`. */
    CASTLE_NODISCARD size_type hash_code() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return info_->hash_code();
    }

    /** @brief Returns whether both indexes refer to the same type. */
    CASTLE_NODISCARD bool operator==(type_index other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return *info_ == *other.info_;
    }

    /** @brief Returns whether the indexes refer to different types. */
    CASTLE_NODISCARD bool operator!=(type_index other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return *info_ != *other.info_;
    }

    /** @brief Orders indexes by `type_info::before()`. */
    CASTLE_NODISCARD bool operator<(type_index other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return info_->before(*other.info_);
    }

    /** @brief Returns whether this index does not order after `other`. */
    CASTLE_NODISCARD bool operator<=(type_index other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return !other.info_->before(*info_);
    }

    /** @brief Returns whether this index orders after `other`. */
    CASTLE_NODISCARD bool operator>(type_index other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return other.info_->before(*info_);
    }

    /** @brief Returns whether this index does not order before `other`. */
    CASTLE_NODISCARD bool operator>=(type_index other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return !info_->before(*other.info_);
    }

private:
    CASTLE_CONST type_info* info_;
};

/** @brief Hash specialization so `type_index` can key Castle hash containers. */
template <>
struct hash<type_index>
{
    /** @brief Returns `value.hash_code()`. */
    size_type operator()(type_index value) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return value.hash_code();
    }
};

} // namespace castle

#endif // CASTLE_UTILITY_TYPE_INDEX_HPP
