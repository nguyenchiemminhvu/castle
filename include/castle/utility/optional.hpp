// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file optional.hpp
 * @brief Allocation-free optional storage for zero or one value.
 *
 * Use this header when a value may be absent and sentinel values are not
 * appropriate. Storage is embedded inside the optional object, object
 * lifetime is managed explicitly, and behavior stays deterministic and
 * exception-free.
 *
 * @code
 * castle::optional<uint32_t> reading;
 * reading.emplace(42U);
 * @endcode
 */
#ifndef CASTLE_UTILITY_OPTIONAL_HPP
#define CASTLE_UTILITY_OPTIONAL_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/types.hpp"
#include "castle/core/traits.hpp"
#include "castle/memory/construct.hpp"
#include "castle/memory/destroy.hpp"
#include "castle/memory/alignment.hpp"
#include "castle/utility/forward.hpp"
#include "castle/utility/move.hpp"

namespace castle
{

/**
 * @brief Tag type used to request an empty `castle::optional`.
 */
struct nullopt_t
{
    explicit CASTLE_CONSTEXPR nullopt_t() CASTLE_NOEXCEPT {}
};

/** @brief Constant tag used to construct or assign an empty optional. */
inline CASTLE_CONSTEXPR nullopt_t nullopt{};

/**
 * @brief Stores zero or one value of type `T` in-place.
 * @tparam T Value type stored by the optional.
 */
template <typename T>
class optional
{
    static_assert(!meta::is_reference<T>::value,
                  "castle::optional<T> requires a non-reference T");
    static_assert(!meta::is_void<T>::value,
                  "castle::optional<void> is not supported");
    static_assert(!meta::is_array<T>::value,
                  "castle::optional<T> does not support array types");
    static_assert(meta::is_object<T>::value,
                  "castle::optional<T> requires an object type");
    static_assert(meta::is_destructible<T>::value,
                  "castle::optional<T> requires a destructible T");

public:
    using value_type = T;
    using size_type = castle::size_type;
    using reference = T&;
    using const_reference = CASTLE_CONST T&;
    using pointer = T*;
    using const_pointer = CASTLE_CONST T*;
    using iterator = T*;
    using const_iterator = CASTLE_CONST T*;
    using storage_type = memory::aligned_storage_as_t<sizeof(T), T>;

    /** @brief Constructs an empty optional. */
    optional() CASTLE_NOEXCEPT : engaged_(false) {}

    /** @brief Constructs an empty optional from `nullopt`. */
    optional(nullopt_t) CASTLE_NOEXCEPT : engaged_(false) {}

    /** @brief Copy-constructs an engaged optional from a value. */
    optional(CASTLE_CONST T& value)
        CASTLE_NOEXCEPT(meta::is_nothrow_constructible<T, CASTLE_CONST T&>::value)
        : engaged_(false)
    {
        construct(value);
    }

    /** @brief Move-constructs an engaged optional from a value. */
    optional(T&& value)
        CASTLE_NOEXCEPT(meta::is_nothrow_constructible<T, T&&>::value)
        : engaged_(false)
    {
        construct(CASTLE_MOVE(value));
    }

    /**
     * @brief Constructs an engaged optional from a compatible forwarded value.
     * @tparam U Source type used to construct `T`.
     * @param value Source value forwarded into the stored object.
     */
    template <typename U,
              meta::enable_if_t<
                  meta::is_constructible<T, U&&>::value &&
                  !meta::is_same<meta::decay_t<U>, optional<T>>::value &&
                  !meta::is_same<meta::decay_t<U>, nullopt_t>::value &&
                  !meta::is_same<meta::decay_t<U>, meta::in_place_t>::value,
                  int> = 0>
    optional(U&& value)
        CASTLE_NOEXCEPT(meta::is_nothrow_constructible<T, U&&>::value)
        : engaged_(false)
    {
        construct(CASTLE_FORWARD<U>(value));
    }

    /**
     * @brief Constructs the contained value in place.
     * @tparam Args Constructor argument types.
     * @param args Arguments forwarded to `T`'s constructor.
     */
    template <typename... Args,
              meta::enable_if_t<meta::is_constructible<T, Args&&...>::value, int> = 0>
    explicit optional(meta::in_place_t, Args&&... args)
        CASTLE_NOEXCEPT(meta::is_nothrow_constructible<T, Args&&...>::value)
        : engaged_(false)
    {
        construct(CASTLE_FORWARD<Args>(args)...);
    }

    /** @brief Copy-constructs an optional, preserving engagement. */
    optional(CASTLE_CONST optional& other)
        CASTLE_NOEXCEPT(meta::is_nothrow_copy_constructible<T>::value)
        : engaged_(false)
    {
        if (other.has_value()) // LCOV_EXCL_BR_LINE
        {
            construct(other.value_ref());
        }
    }

    /** @brief Move-constructs an optional, preserving engagement. */
    optional(optional&& other)
        CASTLE_NOEXCEPT(meta::is_nothrow_move_constructible<T>::value)
        : engaged_(false)
    {
        if (other.has_value()) // LCOV_EXCL_BR_LINE
        {
            construct(CASTLE_MOVE(other.value_ref()));
        }
    }

    /** @brief Destroys the contained value when one is engaged. */
    ~optional() CASTLE_NOEXCEPT
    {
        reset();
    }

    /** @brief Resets the optional to the empty state. */
    optional& operator=(nullopt_t) CASTLE_NOEXCEPT
    {
        reset();
        return *this;
    }

    /** @brief Copy-assigns another optional by reconstructing the stored object. */
    optional& operator=(CASTLE_CONST optional& other)
        CASTLE_NOEXCEPT(meta::is_nothrow_copy_constructible<T>::value)
    {
        if (this != &other) // LCOV_EXCL_BR_LINE
        {
            reset();
            if (other.has_value()) // LCOV_EXCL_BR_LINE
            {
                construct(other.value_ref());
            }
        }
        return *this;
    }

    /** @brief Move-assigns another optional by reconstructing the stored object. */
    optional& operator=(optional&& other)
        CASTLE_NOEXCEPT(meta::is_nothrow_move_constructible<T>::value)
    {
        if (this != &other) // LCOV_EXCL_BR_LINE
        {
            reset();
            if (other.has_value()) // LCOV_EXCL_BR_LINE
            {
                construct(CASTLE_MOVE(other.value_ref()));
            }
        }
        return *this;
    }

    template <typename U,
              meta::enable_if_t<
                  meta::is_constructible<T, U&&>::value &&
                  !meta::is_same<meta::decay_t<U>, optional<T>>::value &&
                  !meta::is_same<meta::decay_t<U>, nullopt_t>::value &&
                  !meta::is_same<meta::decay_t<U>, meta::in_place_t>::value,
                  int> = 0>
    /** @brief Replaces the current state with a value constructed from `value`. */
    optional& operator=(U&& value)
        CASTLE_NOEXCEPT(meta::is_nothrow_constructible<T, U&&>::value)
    {
        reset();
        construct(CASTLE_FORWARD<U>(value));
        return *this;
    }

    /** @brief Returns whether a value is currently engaged. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool has_value() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return engaged_;
    }

    /** @brief Returns whether a value is currently engaged. */
    explicit CASTLE_CONSTEXPR operator bool() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return engaged_;
    }

    /** @brief Returns a reference to the contained value. */
    reference value() & CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(engaged_, CASTLE_ERROR_GENERIC("castle::optional: no value")); // LCOV_EXCL_BR_LINE
        return value_ref();
    }

    /** @brief Returns a const reference to the contained value. */
    const_reference value() CASTLE_CONST & CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(engaged_, CASTLE_ERROR_GENERIC("castle::optional: no value")); // LCOV_EXCL_BR_LINE
        return value_ref();
    }

    /** @brief Returns the contained value as an rvalue reference. */
    T&& value() && CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(engaged_, CASTLE_ERROR_GENERIC("castle::optional: no value")); // LCOV_EXCL_BR_LINE
        return CASTLE_MOVE(value_ref());
    }

    /** @brief Returns the contained const value as an rvalue reference. */
    CASTLE_CONST T&& value() CASTLE_CONST && CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(engaged_, CASTLE_ERROR_GENERIC("castle::optional: no value")); // LCOV_EXCL_BR_LINE
        return CASTLE_MOVE(value_ref());
    }

    /** @brief Dereferences the contained value. */
    reference operator*() & CASTLE_NOEXCEPT
    {
        return value();
    }

    /** @brief Dereferences the contained value. */
    const_reference operator*() CASTLE_CONST & CASTLE_NOEXCEPT
    {
        return value();
    }

    /** @brief Dereferences the contained value as an rvalue. */
    T&& operator*() && CASTLE_NOEXCEPT
    {
        return CASTLE_MOVE(value());
    }

    /** @brief Dereferences the contained const value as an rvalue. */
    CASTLE_CONST T&& operator*() CASTLE_CONST && CASTLE_NOEXCEPT
    {
        return CASTLE_MOVE(value());
    }

    /** @brief Returns a pointer to the contained value. */
    pointer operator->() CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(engaged_, CASTLE_ERROR_GENERIC("castle::optional: no value")); // LCOV_EXCL_BR_LINE
        return storage_.template get_address<T>();
    }

    /** @brief Returns a const pointer to the contained value. */
    const_pointer operator->() CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(engaged_, CASTLE_ERROR_GENERIC("castle::optional: no value")); // LCOV_EXCL_BR_LINE
        return storage_.template get_address<T>();
    }

    /** @brief Returns a pointer to the stored value, or `nullptr` when empty. */
    pointer begin() CASTLE_NOEXCEPT
    {
        return engaged_ ? storage_.template get_address<T>() : nullptr;
    }

    /** @brief Returns a const pointer to the stored value, or `nullptr` when empty. */
    const_pointer begin() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return engaged_ ? storage_.template get_address<T>() : nullptr;
    }

    /** @brief Returns one-past-the-end when engaged, otherwise `nullptr`. */
    pointer end() CASTLE_NOEXCEPT
    {
        return engaged_ ? (storage_.template get_address<T>() + 1U) : nullptr;
    }

    /** @brief Returns one-past-the-end when engaged, otherwise `nullptr`. */
    const_pointer end() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return engaged_ ? (storage_.template get_address<T>() + 1U) : nullptr;
    }

    /** @brief Returns the const begin pointer. */
    const_pointer cbegin() CASTLE_CONST CASTLE_NOEXCEPT { return begin(); }
    /** @brief Returns the const end pointer. */
    const_pointer cend() CASTLE_CONST CASTLE_NOEXCEPT { return end(); }

    /** @brief Destroys the contained value and leaves the optional empty. */
    void reset() CASTLE_NOEXCEPT
    {
        if (engaged_)
        {
            memory::destroy_at(storage_.template get_address<T>());
            engaged_ = false;
        }
    }

    template <typename... Args,
              meta::enable_if_t<meta::is_constructible<T, Args&&...>::value, int> = 0>
    /**
     * @brief Reconstructs the contained value in place.
     * @tparam Args Constructor argument types.
     * @param args Arguments forwarded to `T`'s constructor.
     * @return Reference to the newly constructed value.
     */
    reference emplace(Args&&... args)
        CASTLE_NOEXCEPT(meta::is_nothrow_constructible<T, Args&&...>::value)
    {
        reset();
        construct(CASTLE_FORWARD<Args>(args)...);
        return value_ref();
    }

    template <typename U>
    /** @brief Returns the stored value or a copied fallback. */
    CASTLE_NODISCARD T value_or(U&& default_value) CASTLE_CONST &
        CASTLE_NOEXCEPT(meta::is_nothrow_copy_constructible<T>::value &&
                        meta::is_nothrow_constructible<T, U&&>::value)
    {
        if (engaged_)
        {
            return value_ref();
        }
        return T(CASTLE_FORWARD<U>(default_value));
    }

    template <typename U>
    /** @brief Returns the stored value as an rvalue or a fallback. */
    CASTLE_NODISCARD T value_or(U&& default_value) &&
        CASTLE_NOEXCEPT(meta::is_nothrow_move_constructible<T>::value &&
                        meta::is_nothrow_constructible<T, U&&>::value)
    {
        if (engaged_)
        {
            return CASTLE_MOVE(value_ref());
        }
        return T(CASTLE_FORWARD<U>(default_value));
    }

    /** @brief Exchanges the state of two optionals. */
    void swap(optional& other)
        CASTLE_NOEXCEPT(meta::is_nothrow_move_constructible<T>::value &&
                        meta::is_nothrow_destructible<T>::value)
    {
        // LCOV_EXCL_START
        if (this == &other)
        {
            return;
        }
        // LCOV_EXCL_STOP

        if (engaged_ && other.engaged_) // LCOV_EXCL_BR_LINE
        {
            optional temporary(CASTLE_MOVE(value_ref()));
            reset();
            construct(CASTLE_MOVE(other.value_ref()));
            other.reset();
            other.construct(CASTLE_MOVE(temporary.value_ref()));
        }
        else if (engaged_) // LCOV_EXCL_BR_LINE
        {
            other.construct(CASTLE_MOVE(value_ref()));
            reset();
        }
        // LCOV_EXCL_START
        else if (other.engaged_)
        {
            construct(CASTLE_MOVE(other.value_ref()));
            other.reset();
        }
        // LCOV_EXCL_STOP
    }

private:
    /**
     * Constructs the stored value in-place with the given arguments.
     * @tparam Args The types of the arguments to forward to the constructor.
     * @param args The arguments to forward to the constructor.
     * @note This function sets the engaged state to true.
     */
    template <typename... Args>
    void construct(Args&&... args)
        CASTLE_NOEXCEPT(meta::is_nothrow_constructible<T, Args&&...>::value)
    {
        memory::construct_at<T>(storage_.template get_address<T>(),
                                CASTLE_FORWARD<Args>(args)...);
        engaged_ = true;
    }

    /**
     * Returns a reference to the stored value.
     * @note The optional must be engaged before calling this function.
     * @return A reference to the stored value.
     */
    reference value_ref() CASTLE_NOEXCEPT
    {
        return storage_.template get_reference<T>();
    }

    /**
     * Returns a const reference to the stored value.
     * @note The optional must be engaged before calling this function.
     * @return A const reference to the stored value.
     */
    const_reference value_ref() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return storage_.template get_reference<T>();
    }

    storage_type storage_;
    bool engaged_;
};

/** @brief Returns whether two optionals have equal state and value. */
template <typename T, typename U>
bool operator==(CASTLE_CONST optional<T>& lhs,
                CASTLE_CONST optional<U>& rhs)
{
    if (lhs.has_value() != rhs.has_value())
    {
        return false;
    }
    return !lhs.has_value() || (lhs.value() == rhs.value());
}

/** @brief Returns whether two optionals differ. */
template <typename T, typename U>
bool operator!=(CASTLE_CONST optional<T>& lhs,
                CASTLE_CONST optional<U>& rhs)
{
    return !(lhs == rhs);
}

/** @brief Orders empty optionals before engaged optionals, then compares values. */
template <typename T, typename U>
bool operator<(CASTLE_CONST optional<T>& lhs,
               CASTLE_CONST optional<U>& rhs)
{
    if (!rhs.has_value())
    {
        return false;
    }
    if (!lhs.has_value())
    {
        return true;
    }
    return lhs.value() < rhs.value();
}

/** @brief Returns whether `lhs` sorts after `rhs`. */
template <typename T, typename U>
bool operator>(CASTLE_CONST optional<T>& lhs,
               CASTLE_CONST optional<U>& rhs)
{
    return rhs < lhs;
}

/** @brief Returns whether `lhs` sorts before or equals `rhs`. */
template <typename T, typename U>
bool operator<=(CASTLE_CONST optional<T>& lhs,
                CASTLE_CONST optional<U>& rhs)
{
    return !(rhs < lhs);
}

/** @brief Returns whether `lhs` sorts after or equals `rhs`. */
template <typename T, typename U>
bool operator>=(CASTLE_CONST optional<T>& lhs,
                CASTLE_CONST optional<U>& rhs)
{
    return !(lhs < rhs);
}

/** @brief Returns whether an optional is empty. */
template <typename T>
bool operator==(CASTLE_CONST optional<T>& value, nullopt_t) CASTLE_NOEXCEPT
{
    return !value.has_value();
}

/** @brief Returns whether an optional is empty. */
template <typename T>
bool operator==(nullopt_t, CASTLE_CONST optional<T>& value) CASTLE_NOEXCEPT
{
    return !value.has_value();
}

/** @brief Returns whether an optional is engaged. */
template <typename T>
bool operator!=(CASTLE_CONST optional<T>& value, nullopt_t) CASTLE_NOEXCEPT
{
    return value.has_value();
}

/** @brief Returns whether an optional is engaged. */
template <typename T>
bool operator!=(nullopt_t, CASTLE_CONST optional<T>& value) CASTLE_NOEXCEPT
{
    return value.has_value();
}

/** @brief Returns whether an optional sorts before `nullopt`. */
template <typename T>
bool operator<(CASTLE_CONST optional<T>&, nullopt_t) CASTLE_NOEXCEPT
{
    return false;
}

/** @brief Returns whether `nullopt` sorts before an engaged optional. */
template <typename T>
bool operator<(nullopt_t, CASTLE_CONST optional<T>& value) CASTLE_NOEXCEPT
{
    return value.has_value();
}

/** @brief Returns whether an engaged optional sorts after `nullopt`. */
template <typename T>
bool operator>(CASTLE_CONST optional<T>& value, nullopt_t) CASTLE_NOEXCEPT
{
    return value.has_value();
}

/** @brief Returns whether `nullopt` sorts after an optional. */
template <typename T>
bool operator>(nullopt_t, CASTLE_CONST optional<T>&) CASTLE_NOEXCEPT
{
    return false;
}

/** @brief Returns whether an optional sorts before or equals `nullopt`. */
template <typename T>
bool operator<=(CASTLE_CONST optional<T>&, nullopt_t) CASTLE_NOEXCEPT
{
    return true;
}

/** @brief Returns whether `nullopt` sorts before or equals an optional. */
template <typename T>
bool operator<=(nullopt_t, CASTLE_CONST optional<T>&) CASTLE_NOEXCEPT
{
    return true;
}

/** @brief Returns whether an optional sorts after or equals `nullopt`. */
template <typename T>
bool operator>=(CASTLE_CONST optional<T>& value, nullopt_t) CASTLE_NOEXCEPT
{
    return value.has_value();
}

/** @brief Returns whether `nullopt` sorts after or equals an optional. */
template <typename T>
bool operator>=(nullopt_t, CASTLE_CONST optional<T>&) CASTLE_NOEXCEPT
{
    return true;
}

/** @brief Swaps two optionals. */
template <typename T>
void swap(optional<T>& lhs, optional<T>& rhs)
    CASTLE_NOEXCEPT(CASTLE_NOEXCEPT(lhs.swap(rhs)))
{
    lhs.swap(rhs);
}

/**
 * @brief Creates an engaged optional from a forwarded value.
 * @tparam T Source value type.
 * @param value Value used to initialize the contained object.
 * @return An engaged `optional<meta::decay_t<T>>`.
 */
template <typename T>
optional<meta::decay_t<T>> make_optional(T&& value)
    CASTLE_NOEXCEPT(meta::is_nothrow_constructible<meta::decay_t<T>, T&&>::value)
{
    return optional<meta::decay_t<T>>(CASTLE_FORWARD<T>(value));
}

}

#endif
