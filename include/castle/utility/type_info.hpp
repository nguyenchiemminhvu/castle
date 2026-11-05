// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file type_info.hpp
 * @brief RTTI-free counterpart of `std::type_info` and `typeid`.
 *
 * Use this header when code must identify, compare, order, hash, or name types at runtime
 * (type-erased containers, registries, message routing) on targets built with `-fno-rtti`.
 * Every type has exactly one constant-initialized `castle::type_info` object; `type_id<T>()`
 * returns it. Identity is the object's address, so equality is one pointer compare. Names are
 * extracted from the compiler's function signature at compile time and stored as one
 * null-terminated string per named type. Nothing allocates, throws, or runs at start-up.
 *
 * @code
 * CASTLE_CONST castle::type_info& info = castle::type_id<uint32_t>();
 * bool same = (info == castle::type_id<CASTLE_CONST uint32_t&>()); // true
 * CASTLE_CONST char* text = info.name();                             // "unsigned int" on GCC
 * @endcode
 */
#ifndef CASTLE_UTILITY_TYPE_INFO_HPP
#define CASTLE_UTILITY_TYPE_INFO_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/utility/hash.hpp"

#include <stdint.h>

namespace castle
{

namespace detail
{

template <typename T>
struct type_info_holder;

} // namespace detail

/**
 * @brief Describes one type: identity, implementation-defined order, hash, and name.
 * @note Objects exist only as the per-type instances returned by `type_id<T>()`; they cannot be
 * created, copied, or assigned by users. Compare them by reference, or wrap them in
 * `castle::type_index` to store them in containers.
 */
class type_info
{
public:
    type_info(CASTLE_CONST type_info&) CASTLE_DELETE;
    type_info& operator=(CASTLE_CONST type_info&) CASTLE_DELETE;

    /** @brief Returns whether both objects describe the same type. */
    CASTLE_NODISCARD bool operator==(CASTLE_CONST type_info& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return this == &other;
    }

    /** @brief Returns whether the objects describe different types. */
    CASTLE_NODISCARD bool operator!=(CASTLE_CONST type_info& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return this != &other;
    }

    /**
     * @brief Returns whether this type precedes `other` in an implementation-defined total order.
     * @note The order is fixed for one linked image but may differ between builds.
     */
    CASTLE_NODISCARD bool before(CASTLE_CONST type_info& other) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return reinterpret_cast<uintptr_t>(this) < reinterpret_cast<uintptr_t>(&other);
    }

    /**
     * @brief Returns a hash value that is equal for equal `type_info` objects.
     * @note The value is fixed for one linked image but may differ between builds.
     */
    CASTLE_NODISCARD size_type hash_code() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return castle::hash<CASTLE_CONST type_info*>()(this);
    }

    /**
     * @brief Returns the implementation-defined, null-terminated type name.
     * @return Compiler spelling of the type (e.g. "unsigned int"), or "" when
     * `CASTLE_TYPE_INFO_NAMES` is 0.
     */
    CASTLE_NODISCARD CASTLE_CONST char* name() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return name_;
    }

private:
    template <typename T>
    friend struct detail::type_info_holder;

    explicit CASTLE_CONSTEXPR type_info(CASTLE_CONST char* type_name) CASTLE_NOEXCEPT
        : name_(type_name)
    {
    }

    CASTLE_CONST char* name_;
};

namespace detail
{

#if CASTLE_TYPE_INFO_NAMES

#if defined(__clang__) || defined(__GNUC__)
    #define CASTLE_DETAIL_TYPE_SIGNATURE __PRETTY_FUNCTION__
#else
    #error "CASTLE_TYPE_INFO_NAMES requires __PRETTY_FUNCTION__ or __FUNCSIG__; define it to 0"
#endif

/** @brief Non-terminated view of a compiler-generated function signature. */
struct type_signature_view
{
    CASTLE_CONST char* data;
    size_type size;
};

/// Returns this function's own signature; the only part that varies with `T` is the spelling of `T`.
template <typename T>
CASTLE_CONSTEXPR type_signature_view type_signature() CASTLE_NOEXCEPT
{
    return type_signature_view{ CASTLE_DETAIL_TYPE_SIGNATURE, sizeof(CASTLE_DETAIL_TYPE_SIGNATURE) - 1U };
}

#undef CASTLE_DETAIL_TYPE_SIGNATURE

/** @brief Number of signature characters before and after the spelling of `T`. */
struct type_signature_layout
{
    size_type prefix;
    size_type suffix;
};

/// Measures the layout once with `int` as a probe: the type spelling is the last "int".
CASTLE_CONSTEXPR type_signature_layout probe_type_signature_layout() CASTLE_NOEXCEPT
{
    CASTLE_CONSTEXPR type_signature_view probe = type_signature<int>();
    size_type position = probe.size;
    for (size_type i = 0U; (i + 3U) <= probe.size; ++i)
    {
        if ((probe.data[i] == 'i') && (probe.data[i + 1U] == 'n') && (probe.data[i + 2U] == 't'))
        {
            position = i;
        }
    }
    return type_signature_layout{ position, probe.size - position - 3U };
}

CASTLE_INLINE CASTLE_CONSTEXPR type_signature_layout type_signature_layout_v = probe_type_signature_layout();

static_assert(type_signature_layout_v.prefix < type_signature<int>().size,
              "castle::type_info: unrecognized compiler signature format; define CASTLE_TYPE_INFO_NAMES to 0");

/** @brief Null-terminated character buffer holding one type name. */
template <size_type Length>
struct type_name_buffer
{
    char data[Length + 1U];
};

/// Holds the compile-time extracted, null-terminated name of `T`.
template <typename T>
struct type_name
{
    static CASTLE_CONSTEXPR type_signature_view signature = type_signature<T>();
    static CASTLE_CONSTEXPR size_type length =
        signature.size - type_signature_layout_v.prefix - type_signature_layout_v.suffix;

    static CASTLE_CONSTEXPR type_name_buffer<length> extract() CASTLE_NOEXCEPT
    {
        type_name_buffer<length> buffer{};
        for (size_type i = 0U; i < length; ++i)
        {
            buffer.data[i] = signature.data[type_signature_layout_v.prefix + i];
        }
        buffer.data[length] = '\0';
        return buffer;
    }

    static CASTLE_CONSTEXPR type_name_buffer<length> value = extract();
};

#endif // CASTLE_TYPE_INFO_NAMES

/**
 * @brief Owns the single `type_info` object of `T`.
 * @note The object is deliberately non-const: linkers never fold writable objects, so two types
 * can never share an identity even under identical-data folding. Its constexpr constructor
 * guarantees constant initialization, so no start-up code or init-order hazard exists.
 */
template <typename T>
struct type_info_holder
{
#if CASTLE_TYPE_INFO_NAMES
    static CASTLE_INLINE type_info value{ type_name<T>::value.data };
#else
    static CASTLE_INLINE type_info value{ "" };
#endif
};

} // namespace detail

/**
 * @brief Returns the `type_info` describing `T`, like `typeid(T)`.
 * @tparam T Type to describe. References and top-level cv-qualifiers are ignored, as with
 * `typeid`, so `type_id<CASTLE_CONST int&>() == type_id<int>()`.
 * @return Reference to the unique, statically allocated `type_info` of `T`.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR CASTLE_CONST type_info& type_id() CASTLE_NOEXCEPT
{
    return detail::type_info_holder<meta::remove_cv_t<meta::remove_reference_t<T>>>::value;
}

} // namespace castle

#endif // CASTLE_UTILITY_TYPE_INFO_HPP
