// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file any.hpp
 * @brief Allocation-free, RTTI-free type-erased container for one value of any copyable type.
 *
 * Use this header when one slot must hold a value whose type is chosen at runtime from an
 * open set (configuration parameters, message payloads, user context), but the memory budget
 * must stay fixed. The value always lives inside the object's inline buffer: types that do not
 * fit are rejected at compile time instead of falling back to the heap. Type identity comes
 * from `castle::type_info` instead of RTTI, and the stored value is managed through one pointer
 * to a per-type constant operation table instead of virtual functions.
 *
 * @code
 * castle::any value(42U);
 * if (uint32_t* number = castle::any_cast<uint32_t>(&value))
 * {
 *     *number += 1U;
 * }
 * @endcode
 */
#ifndef CASTLE_UTILITY_ANY_HPP
#define CASTLE_UTILITY_ANY_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/memory/alignment.hpp"
#include "castle/memory/construct.hpp"
#include "castle/memory/destroy.hpp"
#include "castle/memory/object.hpp"
#include "castle/utility/forward.hpp"
#include "castle/utility/move.hpp"
#include "castle/utility/type_info.hpp"

namespace castle
{

template <size_type Capacity, size_type Alignment>
class basic_any;

namespace detail
{

/** @brief Reports whether `T` is a `basic_any` specialization. */
template <typename T>
struct is_basic_any : false_type {};

template <size_type Capacity, size_type Alignment>
struct is_basic_any<basic_any<Capacity, Alignment>> : true_type {};

/**
 * @brief Per-type operation table referenced by every non-empty `basic_any`.
 * @note One table exists per stored type; each `basic_any` keeps only a pointer to it.
 */
struct any_ops
{
    CASTLE_CONST type_info* type;                      ///< Identity of the stored type.
    void (*destroy)(void* object);                     ///< Ends the stored object's lifetime.
    void (*copy)(void* target, CASTLE_CONST void* source); ///< Copy-constructs into raw `target`.
    void (*relocate)(void* target, void* source);      ///< Move-constructs into raw `target`, then destroys `source`.
};

/**
 * @brief Implements and owns the `any_ops` table for one stored type.
 * @tparam T Decayed stored type.
 */
template <typename T>
struct any_handler
{
    static void destroy(void* object) CASTLE_NOEXCEPT
    {
        memory::destroy_at(memory::object_from_address<T>(object));
    }

    static void copy(void* target, CASTLE_CONST void* source)
    {
        memory::construct_at<T>(target, *memory::object_from_address<T>(source));
    }

    static void relocate(void* target, void* source)
    {
        T* object = memory::object_from_address<T>(source);
        memory::construct_at<T>(target, CASTLE_MOVE(*object));
        memory::destroy_at(object);
    }

    static CASTLE_CONSTEXPR any_ops ops = { &type_id<T>(), &destroy, &copy, &relocate };
};

} // namespace detail

/**
 * @brief Holds zero or one value of any copyable type inside a fixed inline buffer.
 * @tparam Capacity Inline buffer size in bytes.
 * @tparam Alignment Inline buffer alignment in bytes; must be a non-zero power of two.
 *
 * Every operation is heap-free and deterministic. A stored type must be a destructible,
 * copy-constructible object type with `sizeof(T) <= Capacity` and `alignof(T) <= Alignment`;
 * anything else fails to compile. Moving an any relocates its value and leaves the source empty.
 */
template <size_type Capacity = castle::inplace_storage_reserved,
          size_type Alignment = castle::inplace_alignment_default>
class basic_any
{
    template <typename T>
    using enable_if_value_t = meta::enable_if_t<
        !detail::is_basic_any<meta::decay_t<T>>::value &&
        !meta::is_specialization_of<meta::decay_t<T>, meta::in_place_type_t>::value,
        int>;

public:
    /** @brief Inline buffer size in bytes. */
    static CASTLE_CONSTEXPR size_type capacity = Capacity;
    /** @brief Inline buffer alignment in bytes. */
    static CASTLE_CONSTEXPR size_type alignment = Alignment;

    using storage_type = memory::aligned_storage_t<Capacity, Alignment>;

    /**
     * @brief Returns whether a value of type `T` can be stored.
     * @tparam T Candidate type; cv-qualifiers are ignored.
     * @return `true` when `T` is a non-array, destructible, copy-constructible object type
     * (other than `basic_any`) that fits the inline buffer's size and alignment.
     */
    template <typename T>
    static CASTLE_CONSTEXPR bool is_supported_type() CASTLE_NOEXCEPT
    {
        using U = meta::remove_cv_t<T>;
        CASTLE_IF_CONSTEXPR (!meta::is_object<U>::value || meta::is_array<U>::value)
        {
            return false;
        }
        else
        {
            return meta::is_destructible<U>::value &&
                   meta::is_copy_constructible<U>::value &&
                   !detail::is_basic_any<U>::value &&
                   (sizeof(U) <= Capacity) &&
                   (alignof(U) <= Alignment);
        }
    }

    /** @brief Constructs an empty any; the inline buffer is left uninitialized. */
    basic_any() CASTLE_NOEXCEPT
        : ops_(nullptr)
    {
    }

    /** @brief Copy-constructs the stored value of `other`, if any. */
    basic_any(CASTLE_CONST basic_any& other)
        : ops_(nullptr)
    {
        if (other.ops_ != nullptr)
        {
            other.ops_->copy(address(), other.address());
            ops_ = other.ops_;
        }
    }

    /** @brief Relocates the stored value of `other`, if any; `other` becomes empty. */
    basic_any(basic_any&& other) CASTLE_NOEXCEPT
        : ops_(nullptr)
    {
        take(other);
    }

    /**
     * @brief Stores a decayed copy of `value`.
     * @tparam T Source type; `meta::decay_t<T>` is stored.
     * @param value Value forwarded into the inline buffer.
     */
    template <typename T, enable_if_value_t<T> = 0>
    basic_any(T&& value)
        : ops_(nullptr)
    {
        emplace<meta::decay_t<T>>(CASTLE_FORWARD<T>(value));
    }

    /**
     * @brief Constructs a `T` in place from `args`.
     * @tparam T Type to construct; cv-qualifiers are ignored.
     * @param args Constructor arguments forwarded to `T`.
     */
    template <typename T, typename... Args>
    explicit basic_any(meta::in_place_type_t<T>, Args&&... args)
        : ops_(nullptr)
    {
        emplace<T>(CASTLE_FORWARD<Args>(args)...);
    }

    /** @brief Destroys the stored value, if any. */
    ~basic_any() CASTLE_NOEXCEPT
    {
        reset();
    }

    /** @brief Replaces the stored value with a copy of `other`'s. */
    basic_any& operator=(CASTLE_CONST basic_any& other)
    {
        if (this != &other)
        {
            replace(basic_any(other));
        }
        return *this;
    }

    /** @brief Replaces the stored value by relocating `other`'s; `other` becomes empty. */
    basic_any& operator=(basic_any&& other) CASTLE_NOEXCEPT
    {
        if (this != &other)
        {
            replace(basic_any(CASTLE_MOVE(other)));
        }
        return *this;
    }

    /** @brief Replaces the stored value with a decayed copy of `value`. */
    template <typename T, enable_if_value_t<T> = 0>
    basic_any& operator=(T&& value)
    {
        replace(basic_any(CASTLE_FORWARD<T>(value)));
        return *this;
    }

    /**
     * @brief Destroys the current value and constructs a `T` in place.
     * @tparam T Type to construct; cv-qualifiers are ignored.
     * @param args Constructor arguments forwarded to `T`.
     * @return Reference to the newly stored value.
     * @warning `args` must not refer into the currently stored value; it is destroyed first.
     */
    template <typename T, typename... Args>
    meta::remove_cv_t<T>& emplace(Args&&... args)
    {
        using U = meta::remove_cv_t<T>;
        static_assert(meta::is_object<U>::value && !meta::is_array<U>::value,
                      "castle::any: stored type must be a non-array object type");
        static_assert(!detail::is_basic_any<U>::value,
                      "castle::any: cannot store a basic_any inside a basic_any");
        static_assert(meta::is_copy_constructible<U>::value,
                      "castle::any: stored type must be copy-constructible");
        static_assert(sizeof(U) <= Capacity,
                      "castle::any: stored type exceeds Capacity; use a larger basic_any");
        static_assert(alignof(U) <= Alignment,
                      "castle::any: stored type alignment exceeds Alignment; use a larger basic_any");

        reset();
        U* object = memory::construct_at<U>(address(), CASTLE_FORWARD<Args>(args)...);
        ops_ = &detail::any_handler<U>::ops;
        return *object;
    }

    /** @brief Destroys the stored value, if any, leaving the any empty. */
    void reset() CASTLE_NOEXCEPT
    {
        if (ops_ != nullptr)
        {
            ops_->destroy(address());
            ops_ = nullptr;
        }
    }

    /** @brief Exchanges the stored values of two anys. */
    void swap(basic_any& other) CASTLE_NOEXCEPT
    {
        if (this != &other)
        {
            basic_any staged(CASTLE_MOVE(other));
            other.take(*this);
            take(staged);
        }
    }

    /** @brief Returns whether a value is stored. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool has_value() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return ops_ != nullptr;
    }

    /** @brief Returns the stored value's `type_info`, or `type_id<void>()` when empty. */
    CASTLE_NODISCARD CASTLE_CONST type_info& type() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (ops_ != nullptr) ? *ops_->type : type_id<void>();
    }

private:
    template <typename T, size_type C, size_type A>
    friend T* any_cast(basic_any<C, A>* operand) CASTLE_NOEXCEPT;

    template <typename T, size_type C, size_type A>
    friend CASTLE_CONST T* any_cast(CASTLE_CONST basic_any<C, A>* operand) CASTLE_NOEXCEPT;

    void* address() CASTLE_NOEXCEPT
    {
        return static_cast<void*>(storage_.data);
    }

    CASTLE_CONST void* address() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<CASTLE_CONST void*>(storage_.data);
    }

    /// Relocates `source`'s value into this empty any; `source` becomes empty.
    void take(basic_any& source) CASTLE_NOEXCEPT
    {
        if (source.ops_ != nullptr)
        {
            source.ops_->relocate(address(), source.address());
            ops_ = source.ops_;
            source.ops_ = nullptr;
        }
    }

    /// Commits a fully built replacement. Building it before `reset()` keeps assignment correct
    /// even when the source lives inside the value being replaced (e.g. nested anys).
    void replace(basic_any&& staged) CASTLE_NOEXCEPT
    {
        reset();
        take(staged);
    }

    storage_type storage_;
    CASTLE_CONST detail::any_ops* ops_;
};

/** @brief Default any sized by Castle's inplace storage configuration. */
using any = basic_any<>;

/**
 * @brief Returns a pointer to the stored value when it has type `T`.
 * @tparam T Requested type; cv-qualifiers are ignored for matching.
 * @param operand Any to inspect, or `nullptr`.
 * @return Pointer to the stored value, or `nullptr` when `operand` is null, empty, or holds another type.
 */
template <typename T, size_type C, size_type A>
CASTLE_NODISCARD T* any_cast(basic_any<C, A>* operand) CASTLE_NOEXCEPT
{
    static_assert(basic_any<C, A>::template is_supported_type<T>(),
                  "castle::any_cast<T>: T can never be stored in this basic_any");
    if ((operand == nullptr) || (operand->type() != type_id<T>()))
    {
        return nullptr;
    }
    return memory::object_from_address<meta::remove_cv_t<T>>(operand->address());
}

/** @brief Returns a const pointer to the stored value when it has type `T`, otherwise `nullptr`. */
template <typename T, size_type C, size_type A>
CASTLE_NODISCARD CASTLE_CONST T* any_cast(CASTLE_CONST basic_any<C, A>* operand) CASTLE_NOEXCEPT
{
    static_assert(basic_any<C, A>::template is_supported_type<T>(),
                  "castle::any_cast<T>: T can never be stored in this basic_any");
    if ((operand == nullptr) || (operand->type() != type_id<T>()))
    {
        return nullptr;
    }
    return memory::object_from_address<meta::remove_cv_t<T>>(operand->address());
}

/**
 * @brief Returns the stored value as `T`, which may be a value or reference type.
 * @tparam T Result type, e.g. `int` (copy), `int&` or `const int&` (reference).
 * @param operand Any whose stored type is `remove_cv_t<remove_reference_t<T>>`.
 * @return The stored value converted to `T`.
 * @warning A type mismatch triggers Castle assertion handling; use the pointer overload to test first.
 */
template <typename T, size_type C, size_type A>
CASTLE_NODISCARD T any_cast(basic_any<C, A>& operand) CASTLE_NOEXCEPT
{
    using U = meta::remove_cv_t<meta::remove_reference_t<T>>;
    static_assert(meta::is_constructible<T, U&>::value,
                  "castle::any_cast<T>: T must be constructible from an lvalue of the stored type");
    U* object = castle::any_cast<U>(&operand);
    CASTLE_ASSERT(object != nullptr, CASTLE_ERROR_GENERIC("castle::any: bad any_cast")); // LCOV_EXCL_BR_LINE
    return static_cast<T>(*object);
}

/** @brief Returns the stored value of a const any as `T`; a type mismatch asserts. */
template <typename T, size_type C, size_type A>
CASTLE_NODISCARD T any_cast(CASTLE_CONST basic_any<C, A>& operand) CASTLE_NOEXCEPT
{
    using U = meta::remove_cv_t<meta::remove_reference_t<T>>;
    static_assert(meta::is_constructible<T, CASTLE_CONST U&>::value,
                  "castle::any_cast<T>: T must be constructible from a const lvalue of the stored type");
    CASTLE_CONST U* object = castle::any_cast<U>(&operand);
    CASTLE_ASSERT(object != nullptr, CASTLE_ERROR_GENERIC("castle::any: bad any_cast")); // LCOV_EXCL_BR_LINE
    return static_cast<T>(*object);
}

/** @brief Returns the stored value of an rvalue any as `T`, moving from it; a type mismatch asserts. */
template <typename T, size_type C, size_type A>
CASTLE_NODISCARD T any_cast(basic_any<C, A>&& operand) CASTLE_NOEXCEPT
{
    using U = meta::remove_cv_t<meta::remove_reference_t<T>>;
    static_assert(meta::is_constructible<T, U>::value,
                  "castle::any_cast<T>: T must be constructible from an rvalue of the stored type");
    U* object = castle::any_cast<U>(&operand);
    CASTLE_ASSERT(object != nullptr, CASTLE_ERROR_GENERIC("castle::any: bad any_cast")); // LCOV_EXCL_BR_LINE
    return static_cast<T>(CASTLE_MOVE(*object));
}

/** @brief Swaps the stored values of two anys. */
template <size_type C, size_type A>
void swap(basic_any<C, A>& lhs, basic_any<C, A>& rhs) CASTLE_NOEXCEPT
{
    lhs.swap(rhs);
}

/**
 * @brief Creates an any holding a `T` constructed in place.
 * @tparam T Type to construct.
 * @tparam Capacity Inline buffer size of the returned any.
 * @tparam Alignment Inline buffer alignment of the returned any.
 * @param args Constructor arguments forwarded to `T`.
 * @return `basic_any<Capacity, Alignment>` holding the new value.
 */
template <typename T,
          size_type Capacity = castle::inplace_storage_reserved,
          size_type Alignment = castle::inplace_alignment_default,
          typename... Args>
CASTLE_NODISCARD basic_any<Capacity, Alignment> make_any(Args&&... args)
{
    return basic_any<Capacity, Alignment>(meta::in_place_type<T>, CASTLE_FORWARD<Args>(args)...);
}

} // namespace castle

#endif // CASTLE_UTILITY_ANY_HPP
