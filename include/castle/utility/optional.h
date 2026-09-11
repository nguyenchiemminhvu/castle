#ifndef CASTLE_UTILITY_OPTIONAL_H
#define CASTLE_UTILITY_OPTIONAL_H

#include "castle/core/compiler.h"
#include "castle/core/error_handler.h"
#include "castle/core/types.h"
#include "castle/core/traits.h"
#include "castle/memory/construct.h"
#include "castle/memory/destroy.h"
#include "castle/memory/alignment.h"
#include "castle/utility/forward.h"
#include "castle/utility/move.h"

namespace castle
{
// ============================================================================
// nullopt_t / nullopt
//
// Tag used to explicitly construct or reset an optional without a value.
// ============================================================================
struct nullopt_t
{
    explicit CASTLE_CONSTEXPR nullopt_t() CASTLE_NOEXCEPT {}
};

inline CASTLE_CONSTEXPR nullopt_t nullopt{};

// ============================================================================
// optional
//
// Allocation-free, single-object optional value.
//
// An empty optional contains no live T object. The storage is reused by
// emplace()/reset() without allocating from a heap.
// ============================================================================
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

    // ------------------------------------------------------------------------
    // Construction.
    // ------------------------------------------------------------------------
    optional() CASTLE_NOEXCEPT : engaged_(false) {}

    optional(nullopt_t) CASTLE_NOEXCEPT : engaged_(false) {}

    optional(CASTLE_CONST T& value)
        CASTLE_NOEXCEPT(meta::is_nothrow_constructible<T, CASTLE_CONST T&>::value)
        : engaged_(false)
    {
        construct(value);
    }

    optional(T&& value)
        CASTLE_NOEXCEPT(meta::is_nothrow_constructible<T, T&&>::value)
        : engaged_(false)
    {
        construct(CASTLE_MOVE(value));
    }

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

    template <typename... Args,
              meta::enable_if_t<meta::is_constructible<T, Args&&...>::value, int> = 0>
    explicit optional(meta::in_place_t, Args&&... args)
        CASTLE_NOEXCEPT(meta::is_nothrow_constructible<T, Args&&...>::value)
        : engaged_(false)
    {
        construct(CASTLE_FORWARD<Args>(args)...);
    }

    optional(CASTLE_CONST optional& other)
        CASTLE_NOEXCEPT(meta::is_nothrow_copy_constructible<T>::value)
        : engaged_(false)
    {
        if (other.has_value())
        {
            construct(other.value_ref());
        }
    }

    optional(optional&& other)
        CASTLE_NOEXCEPT(meta::is_nothrow_move_constructible<T>::value)
        : engaged_(false)
    {
        if (other.has_value())
        {
            construct(CASTLE_MOVE(other.value_ref()));
        }
    }

    ~optional() CASTLE_NOEXCEPT
    {
        reset();
    }

    // ------------------------------------------------------------------------
    // Assignment.
    //
    // Assignment reconstructs the stored object instead of requiring T to be
    // assignable. This keeps optional useful for move-only and non-assignable
    // embedded value types while keeping the lifetime rules explicit.
    // ------------------------------------------------------------------------
    optional& operator=(nullopt_t) CASTLE_NOEXCEPT
    {
        reset();
        return *this;
    }

    optional& operator=(CASTLE_CONST optional& other)
        CASTLE_NOEXCEPT(meta::is_nothrow_copy_constructible<T>::value)
    {
        if (this != &other)
        {
            reset();
            if (other.has_value())
            {
                construct(other.value_ref());
            }
        }
        return *this;
    }

    optional& operator=(optional&& other)
        CASTLE_NOEXCEPT(meta::is_nothrow_move_constructible<T>::value)
    {
        if (this != &other)
        {
            reset();
            if (other.has_value())
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
    optional& operator=(U&& value)
        CASTLE_NOEXCEPT(meta::is_nothrow_constructible<T, U&&>::value)
    {
        reset();
        construct(CASTLE_FORWARD<U>(value));
        return *this;
    }

    // ------------------------------------------------------------------------
    // State.
    // ------------------------------------------------------------------------
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool has_value() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return engaged_;
    }

    explicit CASTLE_CONSTEXPR operator bool() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return engaged_;
    }

    // ------------------------------------------------------------------------
    // Access.
    //
    // value()/operator*/operator-> use CASTLE_ASSERT for an empty optional.
    // The configured CASTLE error policy therefore decides whether the fault
    // aborts, is logged, or becomes a no-op in a release build.
    // ------------------------------------------------------------------------
    reference value() & CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(engaged_, CASTLE_ERROR_GENERIC("castle::optional: no value"));
        return value_ref();
    }

    const_reference value() CASTLE_CONST & CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(engaged_, CASTLE_ERROR_GENERIC("castle::optional: no value"));
        return value_ref();
    }

    T&& value() && CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(engaged_, CASTLE_ERROR_GENERIC("castle::optional: no value"));
        return CASTLE_MOVE(value_ref());
    }

    CASTLE_CONST T&& value() CASTLE_CONST && CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(engaged_, CASTLE_ERROR_GENERIC("castle::optional: no value"));
        return CASTLE_MOVE(value_ref());
    }

    reference operator*() & CASTLE_NOEXCEPT
    {
        return value();
    }

    const_reference operator*() CASTLE_CONST & CASTLE_NOEXCEPT
    {
        return value();
    }

    T&& operator*() && CASTLE_NOEXCEPT
    {
        return CASTLE_MOVE(value());
    }

    CASTLE_CONST T&& operator*() CASTLE_CONST && CASTLE_NOEXCEPT
    {
        return CASTLE_MOVE(value());
    }

    pointer operator->() CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(engaged_, CASTLE_ERROR_GENERIC("castle::optional: no value"));
        return storage_.template get_address<T>();
    }

    const_pointer operator->() CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT(engaged_, CASTLE_ERROR_GENERIC("castle::optional: no value"));
        return storage_.template get_address<T>();
    }

    pointer begin() CASTLE_NOEXCEPT
    {
        return engaged_ ? storage_.template get_address<T>() : nullptr;
    }

    const_pointer begin() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return engaged_ ? storage_.template get_address<T>() : nullptr;
    }

    pointer end() CASTLE_NOEXCEPT
    {
        return engaged_ ? (storage_.template get_address<T>() + 1U) : nullptr;
    }

    const_pointer end() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return engaged_ ? (storage_.template get_address<T>() + 1U) : nullptr;
    }

    const_pointer cbegin() CASTLE_CONST CASTLE_NOEXCEPT { return begin(); }
    const_pointer cend() CASTLE_CONST CASTLE_NOEXCEPT { return end(); }

    // ------------------------------------------------------------------------
    // Value management.
    // ------------------------------------------------------------------------
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
    reference emplace(Args&&... args)
        CASTLE_NOEXCEPT(meta::is_nothrow_constructible<T, Args&&...>::value)
    {
        reset();
        construct(CASTLE_FORWARD<Args>(args)...);
        return value_ref();
    }

    template <typename U>
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

    // ------------------------------------------------------------------------
    // Swap.
    // ------------------------------------------------------------------------
    void swap(optional& other)
        CASTLE_NOEXCEPT(meta::is_nothrow_move_constructible<T>::value &&
                        meta::is_nothrow_destructible<T>::value)
    {
        if (this == &other)
        {
            return;
        }

        if (engaged_ && other.engaged_)
        {
            optional temporary(CASTLE_MOVE(value_ref()));
            reset();
            construct(CASTLE_MOVE(other.value_ref()));
            other.reset();
            other.construct(CASTLE_MOVE(temporary.value_ref()));
        }
        else if (engaged_)
        {
            other.construct(CASTLE_MOVE(value_ref()));
            reset();
        }
        else if (other.engaged_)
        {
            construct(CASTLE_MOVE(other.value_ref()));
            other.reset();
        }
    }

private:
    template <typename... Args>
    void construct(Args&&... args)
        CASTLE_NOEXCEPT(meta::is_nothrow_constructible<T, Args&&...>::value)
    {
        memory::construct_at<T>(storage_.template get_address<T>(),
                                CASTLE_FORWARD<Args>(args)...);
        engaged_ = true;
    }

    reference value_ref() CASTLE_NOEXCEPT
    {
        return storage_.template get_reference<T>();
    }

    const_reference value_ref() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return storage_.template get_reference<T>();
    }

    storage_type storage_;
    bool engaged_;
};

// ============================================================================
// optional comparisons
// ============================================================================
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

template <typename T, typename U>
bool operator!=(CASTLE_CONST optional<T>& lhs,
                CASTLE_CONST optional<U>& rhs)
{
    return !(lhs == rhs);
}

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

template <typename T, typename U>
bool operator>(CASTLE_CONST optional<T>& lhs,
               CASTLE_CONST optional<U>& rhs)
{
    return rhs < lhs;
}

template <typename T, typename U>
bool operator<=(CASTLE_CONST optional<T>& lhs,
                CASTLE_CONST optional<U>& rhs)
{
    return !(rhs < lhs);
}

template <typename T, typename U>
bool operator>=(CASTLE_CONST optional<T>& lhs,
                CASTLE_CONST optional<U>& rhs)
{
    return !(lhs < rhs);
}

template <typename T>
bool operator==(CASTLE_CONST optional<T>& value, nullopt_t) CASTLE_NOEXCEPT
{
    return !value.has_value();
}

template <typename T>
bool operator==(nullopt_t, CASTLE_CONST optional<T>& value) CASTLE_NOEXCEPT
{
    return !value.has_value();
}

template <typename T>
bool operator!=(CASTLE_CONST optional<T>& value, nullopt_t) CASTLE_NOEXCEPT
{
    return value.has_value();
}

template <typename T>
bool operator!=(nullopt_t, CASTLE_CONST optional<T>& value) CASTLE_NOEXCEPT
{
    return value.has_value();
}

template <typename T>
bool operator<(CASTLE_CONST optional<T>&, nullopt_t) CASTLE_NOEXCEPT
{
    return false;
}

template <typename T>
bool operator<(nullopt_t, CASTLE_CONST optional<T>& value) CASTLE_NOEXCEPT
{
    return value.has_value();
}

template <typename T>
bool operator>(CASTLE_CONST optional<T>& value, nullopt_t) CASTLE_NOEXCEPT
{
    return value.has_value();
}

template <typename T>
bool operator>(nullopt_t, CASTLE_CONST optional<T>&) CASTLE_NOEXCEPT
{
    return false;
}

template <typename T>
bool operator<=(CASTLE_CONST optional<T>&, nullopt_t) CASTLE_NOEXCEPT
{
    return true;
}

template <typename T>
bool operator<=(nullopt_t, CASTLE_CONST optional<T>&) CASTLE_NOEXCEPT
{
    return true;
}

template <typename T>
bool operator>=(CASTLE_CONST optional<T>& value, nullopt_t) CASTLE_NOEXCEPT
{
    return value.has_value();
}

template <typename T>
bool operator>=(nullopt_t, CASTLE_CONST optional<T>&) CASTLE_NOEXCEPT
{
    return true;
}

// ============================================================================
// swap(optional, optional)
// ============================================================================
template <typename T>
void swap(optional<T>& lhs, optional<T>& rhs)
    CASTLE_NOEXCEPT(CASTLE_NOEXCEPT(lhs.swap(rhs)))
{
    lhs.swap(rhs);
}

// ============================================================================
// make_optional
// ============================================================================
template <typename T>
optional<meta::decay_t<T>> make_optional(T&& value)
    CASTLE_NOEXCEPT(meta::is_nothrow_constructible<meta::decay_t<T>, T&&>::value)
{
    return optional<meta::decay_t<T>>(CASTLE_FORWARD<T>(value));
}

} // namespace castle

#endif // CASTLE_UTILITY_OPTIONAL_H
