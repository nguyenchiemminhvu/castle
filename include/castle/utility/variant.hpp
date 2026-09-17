// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file variant.hpp
 * @brief Fixed-size discriminated union for Castle.
 *
 * Use this header when one object must store exactly one of several
 * alternative types while keeping storage size, lifetime transitions, and
 * dispatch explicit. Storage is embedded, no allocation occurs, and runtime
 * dispatch stays table-free.
 *
 * @code
 * castle::variant<uint32_t, uint16_t> value(castle::in_place_index<0>, 7U);
 * auto current = castle::get<uint32_t>(value);
 * @endcode
 */
#ifndef CASTLE_UTILITY_VARIANT_HPP
#define CASTLE_UTILITY_VARIANT_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/types.hpp"
#include "castle/core/type_ranges.hpp"
#include "castle/core/traits.hpp"
#include "castle/memory/construct.hpp"
#include "castle/memory/destroy.hpp"
#include "castle/memory/alignment.hpp"
#include "castle/utility/forward.hpp"
#include "castle/utility/move.hpp"
#include "castle/utility/swap.hpp"

namespace castle
{





/**
 * @brief Tag type used to construct a variant alternative by compile-time index.
 * @tparam Index Alternative index to activate.
 */
template <size_type Index>
struct in_place_index_t
{
    explicit CASTLE_CONSTEXPR in_place_index_t() CASTLE_NOEXCEPT {}
};

/**
 * @brief Constant tag used to construct a variant alternative by compile-time index.
 * @tparam Index Alternative index to activate.
 */
template <size_type Index>
inline CASTLE_CONSTEXPR in_place_index_t<Index> in_place_index{};








/** @brief Sentinel index used when no variant alternative is active. */
static CASTLE_CONSTEXPR size_type variant_npos = castle::numeric_limits<size_type>().max();

namespace detail
{




template <typename T, typename... Ts>
struct variant_type_index;

template <typename T>
struct variant_type_index<T>
    : integral_constant<size_type, variant_npos>
{
};

template <typename T, typename Head, typename... Tail>
struct variant_type_index<T, Head, Tail...>
    : integral_constant<size_type,
                       meta::is_same<T, Head>::value
                           ? 0U
                           : (variant_type_index<T, Tail...>::value == variant_npos
                                  ? variant_npos
                                  : 1U + variant_type_index<T, Tail...>::value)>
{
};

template <size_type Index, typename... Ts>
struct variant_type_at;

template <size_type Index, typename Head, typename... Tail>
struct variant_type_at<Index, Head, Tail...>
    : variant_type_at<Index - 1U, Tail...>
{
};

template <typename Head, typename... Tail>
struct variant_type_at<0U, Head, Tail...>
{
    using type = Head;
};

template <size_type Index, typename... Ts>
using variant_type_at_t = typename variant_type_at<Index, Ts...>::type;

template <typename... Ts>
struct variant_max_size;

template <typename T>
struct variant_max_size<T>
    : integral_constant<size_type, sizeof(T)>
{
};

template <typename Head, typename... Tail>
struct variant_max_size<Head, Tail...>
    : integral_constant<size_type,
                       (sizeof(Head) > variant_max_size<Tail...>::value)
                           ? sizeof(Head)
                           : variant_max_size<Tail...>::value>
{
};

template <typename... Ts>
struct variant_max_align;

template <typename T>
struct variant_max_align<T>
    : integral_constant<size_type, alignof(T)>
{
};

template <typename Head, typename... Tail>
struct variant_max_align<Head, Tail...>
    : integral_constant<size_type,
                       (alignof(Head) > variant_max_align<Tail...>::value)
                           ? alignof(Head)
                           : variant_max_align<Tail...>::value>
{
};

template <typename T, typename... Ts>
struct variant_has_type
    : bool_constant<(variant_type_index<T, Ts...>::value != variant_npos)>
{
};

template <typename Visitor, typename... Args>
struct variant_visit_invocable
    : meta::conjunction<meta::is_invocable<Visitor, Args>...>
{
};

} 










/**
 * @brief Stores exactly one value from a fixed set of alternative types.
 * @tparam Ts Alternative types that may be stored.
 */
template <typename... Ts>
class variant
{
    static_assert(sizeof...(Ts) > 0U,
                  "castle::variant requires at least one alternative");
    static_assert(meta::has_unique_types<Ts...>::value,
                  "castle::variant requires unique alternative types");
    static_assert(meta::conjunction<meta::is_object<Ts>...>::value,
                  "castle::variant alternatives must be object types");
    static_assert(meta::conjunction<meta::negation<meta::is_array<Ts>>...>::value,
                  "castle::variant alternatives must not be arrays");
    static_assert(meta::conjunction<meta::is_destructible<Ts>...>::value,
                  "castle::variant alternatives must be destructible");

public:
    static CASTLE_CONSTEXPR size_type size = sizeof...(Ts);
    static CASTLE_CONSTEXPR size_type storage_size = detail::variant_max_size<Ts...>::value;
    static CASTLE_CONSTEXPR size_type storage_alignment = detail::variant_max_align<Ts...>::value;

    template <size_type Index>
    using alternative_type = detail::variant_type_at_t<Index, Ts...>;
    using storage_type = memory::aligned_storage_t<storage_size, storage_alignment>;

    
    
    
    /** @brief Default-constructs the first alternative. */
    variant()
        CASTLE_NOEXCEPT(meta::is_nothrow_default_constructible<alternative_type<0U>>::value)
        : index_(variant_npos)
    {
        emplace<0U>();
    }


    /**
     * @brief Constructs a specific alternative by index.
     * @tparam Index Alternative index to construct.
     * @tparam Args Constructor argument types.
     * @param args Arguments forwarded to the selected alternative.
     */
    template <size_type Index, typename... Args,
              meta::enable_if_t<
                  (Index < sizeof...(Ts)) &&
                  meta::is_constructible<alternative_type<Index>, Args&&...>::value,
                  int> = 0>
    explicit variant(in_place_index_t<Index>, Args&&... args)
        CASTLE_NOEXCEPT(meta::is_nothrow_constructible<alternative_type<Index>, Args&&...>::value)
        : index_(variant_npos)
    {
        emplace<Index>(CASTLE_FORWARD<Args>(args)...);
    }

    template <typename T, typename... Args,
              typename U = meta::remove_cv_t<T>,
              meta::enable_if_t<
                  detail::variant_has_type<U, Ts...>::value &&
                  meta::is_constructible<U, Args&&...>::value,
                  int> = 0>
    /**
     * @brief Constructs a specific alternative by type.
     * @tparam T Alternative type to construct.
     * @tparam Args Constructor argument types.
     * @param args Arguments forwarded to the selected alternative.
     */
    explicit variant(meta::in_place_type_t<T>, Args&&... args)
        CASTLE_NOEXCEPT(meta::is_nothrow_constructible<U, Args&&...>::value)
        : index_(variant_npos)
    {
        emplace<U>(CASTLE_FORWARD<Args>(args)...);
    }

    template <typename T,
              typename U = meta::decay_t<T>,
              meta::enable_if_t<
                  detail::variant_has_type<U, Ts...>::value &&
                  !meta::is_same<U, variant>::value &&
                  meta::is_constructible<U, T&&>::value,
                  int> = 0>
    /** @brief Constructs the variant from a value whose decayed type is one of the alternatives. */
    variant(T&& value)
        CASTLE_NOEXCEPT(meta::is_nothrow_constructible<U, T&&>::value)
        : index_(variant_npos)
    {
        emplace<U>(CASTLE_FORWARD<T>(value));
    }

    /** @brief Copy-constructs the currently active alternative. */
    variant(CASTLE_CONST variant& other)
        CASTLE_NOEXCEPT(meta::conjunction<meta::is_nothrow_copy_constructible<Ts>...>::value)
        : index_(variant_npos)
    {
        copy_from(other);
    }

    /** @brief Move-constructs the currently active alternative. */
    variant(variant&& other)
        CASTLE_NOEXCEPT(meta::conjunction<meta::is_nothrow_move_constructible<Ts>...>::value)
        : index_(variant_npos)
    {
        move_from<0U>(other);
    }

    /** @brief Destroys the active alternative, if any. */
    ~variant() CASTLE_NOEXCEPT
    {
        reset();
    }

    
    
    
    /** @brief Copy-assigns another variant by reconstructing its active alternative. */
    variant& operator=(CASTLE_CONST variant& other)
        CASTLE_NOEXCEPT(meta::conjunction<meta::is_nothrow_copy_constructible<Ts>...>::value)
    {
        if (this != &other) // LCOV_EXCL_BR_LINE
        {
            reset();
            copy_from(other);
        }
        return *this;
    }

    /** @brief Move-assigns another variant by reconstructing its active alternative. */
    variant& operator=(variant&& other)
        CASTLE_NOEXCEPT(meta::conjunction<meta::is_nothrow_move_constructible<Ts>...>::value)
    {
        if (this != &other) // LCOV_EXCL_BR_LINE
        {
            reset();
            move_from<0U>(other);
        }
        return *this;
    }

    template <typename T,
              typename U = meta::decay_t<T>,
              meta::enable_if_t<
                  detail::variant_has_type<U, Ts...>::value &&
                  meta::is_constructible<U, T&&>::value,
                  int> = 0>
    /** @brief Replaces the active alternative with a value-compatible alternative. */
    variant& operator=(T&& value)
        CASTLE_NOEXCEPT(meta::is_nothrow_constructible<U, T&&>::value)
    {
        emplace<U>(CASTLE_FORWARD<T>(value));
        return *this;
    }

    
    
    
    /** @brief Returns the index of the active alternative. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR size_type index() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return index_;
    }

    /** @brief Returns whether the variant currently holds no active alternative. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool valueless_by_exception() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return index_ == variant_npos;
    }

    
    
    
    /** @brief Destroys the active alternative and leaves the variant empty. */
    void reset() CASTLE_NOEXCEPT
    {
        if (index_ != variant_npos)
        {
            destroy_active<0U>();
            index_ = variant_npos;
        }
    }

    
    
    
    template <size_type Index, typename... Args,
              meta::enable_if_t<
                  (Index < sizeof...(Ts)) &&
                  meta::is_constructible<alternative_type<Index>, Args&&...>::value,
                  int> = 0>
    /**
     * @brief Reconstructs the variant in place using an alternative index.
     * @tparam Index Alternative index to activate.
     * @tparam Args Constructor argument types.
     * @param args Arguments forwarded to the selected alternative.
     * @return Reference to the newly constructed alternative.
     */
    alternative_type<Index>& emplace(Args&&... args)
        CASTLE_NOEXCEPT(meta::is_nothrow_constructible<alternative_type<Index>, Args&&...>::value)
    {
        reset();
        memory::construct_at<alternative_type<Index>>(
            storage_.template get_address<alternative_type<Index>>(),
            CASTLE_FORWARD<Args>(args)...
        );
        index_ = Index;
        return get_value<Index>();
    }

    template <typename T, typename... Args,
              typename U = meta::remove_cv_t<T>,
              meta::enable_if_t<
                  detail::variant_has_type<U, Ts...>::value &&
                  meta::is_constructible<U, Args&&...>::value,
                  int> = 0>
    /**
     * @brief Reconstructs the variant in place using an alternative type.
     * @tparam T Alternative type to activate.
     * @tparam Args Constructor argument types.
     * @param args Arguments forwarded to the selected alternative.
     * @return Reference to the newly constructed alternative.
     */
    U& emplace(Args&&... args)
        CASTLE_NOEXCEPT(meta::is_nothrow_constructible<U, Args&&...>::value)
    {
        return emplace<detail::variant_type_index<U, Ts...>::value>(
            CASTLE_FORWARD<Args>(args)...
        );
    }

    
    
    
    template <typename T>
    /** @brief Returns whether `T` is one of the variant alternatives. */
    static CASTLE_CONSTEXPR bool is_supported_type() CASTLE_NOEXCEPT
    {
        using U = meta::remove_cv_t<T>;
        return detail::variant_has_type<U, Ts...>::value;
    }

    
    
    
    /** @brief Exchanges the active alternatives of two variants. */
    void swap(variant& other)
        CASTLE_NOEXCEPT(meta::conjunction<meta::is_nothrow_move_constructible<Ts>...>::value)
    {
        // LCOV_EXCL_START
        if (this == &other)
        {
            return;
        }
        // LCOV_EXCL_STOP

        variant temporary(CASTLE_MOVE(*this));
        *this = CASTLE_MOVE(other);
        other = CASTLE_MOVE(temporary);
    }

    
    /**
     * @brief Returns the active alternative by index without checking.
     * @note This function is public so non-member helpers can implement `get` and `visit`.
     */
    template <size_type Index>
    alternative_type<Index>& get_value() & CASTLE_NOEXCEPT
    {
        return storage_.template get_reference<alternative_type<Index>>();
    }

    /** @brief Returns the active alternative by index without checking. */
    template <size_type Index>
    CASTLE_CONST alternative_type<Index>& get_value() CASTLE_CONST & CASTLE_NOEXCEPT
    {
        return storage_.template get_reference<alternative_type<Index>>();
    }

    /** @brief Returns the active alternative by index as an rvalue reference. */
    template <size_type Index>
    alternative_type<Index>&& get_value() && CASTLE_NOEXCEPT
    {
        return CASTLE_MOVE(get_value<Index>());
    }

    /** @brief Returns the active const alternative by index as an rvalue reference. */
    template <size_type Index>
    CASTLE_CONST alternative_type<Index>&& get_value() CASTLE_CONST && CASTLE_NOEXCEPT
    {
        return CASTLE_MOVE(get_value<Index>());
    }

    
    template <typename Visitor, typename Variant, typename Return, size_type Index>
    static Return visit_impl(Visitor&& visitor, Variant&& value)
    {
        if (value.index() == Index)
        {
            CASTLE_IF_CONSTEXPR (meta::is_void<Return>::value)
            {
                CASTLE_FORWARD<Visitor>(visitor)(
                    CASTLE_FORWARD<Variant>(value).template get_value<Index>()
                );
                return;
            }
            else
            {
                return CASTLE_FORWARD<Visitor>(visitor)(
                    CASTLE_FORWARD<Variant>(value).template get_value<Index>()
                );
            }
        }

        // Dispatch is expanded as a compile-time linear chain, avoiding jump
        // tables, function pointers, and heap-backed visitation state.
        CASTLE_IF_CONSTEXPR (Index + 1U < sizeof...(Ts))
        {
            return visit_impl<Visitor, Variant, Return, Index + 1U>(
                CASTLE_FORWARD<Visitor>(visitor),
                CASTLE_FORWARD<Variant>(value)
            );
        }

        CASTLE_ASSERT_FAIL(CASTLE_ERROR_GENERIC("castle::variant: invalid visit")); // LCOV_EXCL_LINE
        CASTLE_UNREACHABLE();
        CASTLE_IF_CONSTEXPR (!meta::is_void<Return>::value)
        {
            using return_value_type = meta::remove_reference_t<Return>;
            return static_cast<Return>(*static_cast<return_value_type*>(nullptr));
        }
    }

    storage_type storage_;
    size_type index_;

private:

    template <size_type Index>
    void destroy_active() CASTLE_NOEXCEPT
    {
        if (index_ == Index)
        {
            memory::destroy_at(storage_.template get_address<alternative_type<Index>>());
            return;
        }

        CASTLE_IF_CONSTEXPR (Index + 1U < sizeof...(Ts))
        {
            destroy_active<Index + 1U>();
        }
    }

    template <size_type Index>
    void copy_from_impl(CASTLE_CONST variant& other)
    {
        if (other.index_ == Index)
        {
            emplace<Index>(other.template get_value<Index>());
            return;
        }

        CASTLE_IF_CONSTEXPR (Index + 1U < sizeof...(Ts))
        {
            copy_from_impl<Index + 1U>(other);
        }
    }

    void copy_from(CASTLE_CONST variant& other)
    {
        if (other.index_ != variant_npos) // LCOV_EXCL_BR_LINE
        {
            copy_from_impl<0U>(other);
        }
    }

    template <size_type Index>
    void move_from(variant& other)
    {
        if (other.index_ == Index)
        {
            emplace<Index>(CASTLE_MOVE(other.template get_value<Index>()));
            return;
        }

        CASTLE_IF_CONSTEXPR (Index + 1U < sizeof...(Ts))
        {
            move_from<Index + 1U>(other);
        }
    }
};




/**
 * @brief Maps a variant index to the corresponding alternative type.
 * @tparam Index Zero-based alternative index.
 * @tparam Variant Variant type to inspect.
 */
template <size_type Index, typename Variant>
struct variant_alternative;

template <size_type Index, typename... Ts>
struct variant_alternative<Index, variant<Ts...>>
{
    using type = detail::variant_type_at_t<Index, Ts...>;
};

template <size_type Index, typename... Ts>
struct variant_alternative<Index, CASTLE_CONST variant<Ts...>>
{
    using type = CASTLE_CONST detail::variant_type_at_t<Index, Ts...>;
};

/**
 * @brief Convenience alias for `variant_alternative<Index, Variant>::type`.
 * @tparam Index Zero-based alternative index.
 * @tparam Variant Variant type to inspect.
 */
template <size_type Index, typename Variant>
using variant_alternative_t = typename variant_alternative<Index, Variant>::type;

/**
 * @brief Returns the compile-time number of alternatives in a variant.
 * @tparam Variant Variant type to inspect.
 */
template <typename Variant>
struct variant_size;

template <typename... Ts>
struct variant_size<variant<Ts...>>
    : integral_constant<size_type, sizeof...(Ts)>
{
};

/**
 * @brief Convenience variable template exposing `variant_size<Variant>::value`.
 * @tparam Variant Variant type to inspect.
 */
template <typename Variant>
inline CASTLE_CONSTEXPR size_type variant_size_v = variant_size<Variant>::value;




/**
 * @brief Returns whether the active alternative has type `T`.
 * @tparam T Alternative type to test.
 * @tparam Ts Variant alternative types.
 * @param value Variant to inspect.
 * @return `true` when `T` is active.
 */
template <typename T, typename... Ts>
CASTLE_NODISCARD CASTLE_CONSTEXPR bool
holds_alternative(CASTLE_CONST variant<Ts...>& value) CASTLE_NOEXCEPT
{
    using U = meta::remove_cv_t<T>;
    static_assert(detail::variant_has_type<U, Ts...>::value,
                  "castle::holds_alternative<T>: T is not an alternative");
    return value.index() == detail::variant_type_index<U, Ts...>::value;
}

/** @brief Returns whether the active alternative has index `Index`. */
template <size_type Index, typename... Ts>
CASTLE_NODISCARD CASTLE_CONSTEXPR bool
holds_alternative(CASTLE_CONST variant<Ts...>& value) CASTLE_NOEXCEPT
{
    static_assert(Index < sizeof...(Ts),
                  "castle::holds_alternative<Index>: index out of range");
    return value.index() == Index;
}




/**
 * @brief Returns the active alternative by index.
 * @tparam Index Alternative index to retrieve.
 * @tparam Ts Variant alternative types.
 * @param value Variant to inspect.
 * @return Reference to the active alternative.
 * @warning An index mismatch triggers Castle assertion handling.
 */
template <size_type Index, typename... Ts>
CASTLE_CONSTEXPR variant_alternative_t<Index, variant<Ts...>>&
get(variant<Ts...>& value) CASTLE_NOEXCEPT
{
    static_assert(Index < sizeof...(Ts),
                  "castle::get<Index>: index out of range");
    CASTLE_ASSERT(value.index() == Index, // LCOV_EXCL_BR_LINE
                  CASTLE_ERROR_GENERIC("castle::variant: incorrect alternative"));
    return value.template get_value<Index>();
}

/** @brief Returns the active const alternative by index. */
template <size_type Index, typename... Ts>
CASTLE_CONSTEXPR CASTLE_CONST variant_alternative_t<Index, variant<Ts...>>&
get(CASTLE_CONST variant<Ts...>& value) CASTLE_NOEXCEPT
{
    static_assert(Index < sizeof...(Ts),
                  "castle::get<Index>: index out of range");
    CASTLE_ASSERT(value.index() == Index,
                  CASTLE_ERROR_GENERIC("castle::variant: incorrect alternative"));
    return value.template get_value<Index>();
}

/** @brief Returns the active alternative by index as an rvalue reference. */
template <size_type Index, typename... Ts>
CASTLE_CONSTEXPR variant_alternative_t<Index, variant<Ts...>>&&
get(variant<Ts...>&& value) CASTLE_NOEXCEPT
{
    return CASTLE_MOVE(castle::get<Index>(value));
}

/** @brief Returns the active const alternative by index as an rvalue reference. */
template <size_type Index, typename... Ts>
CASTLE_CONSTEXPR CASTLE_CONST variant_alternative_t<Index, variant<Ts...>>&&
get(CASTLE_CONST variant<Ts...>&& value) CASTLE_NOEXCEPT
{
    return CASTLE_MOVE(castle::get<Index>(value));
}

/** @brief Returns the active alternative by type. */
template <typename T, typename... Ts>
CASTLE_CONSTEXPR T&
get(variant<Ts...>& value) CASTLE_NOEXCEPT
{
    using U = meta::remove_cv_t<T>;
    static_assert(detail::variant_has_type<U, Ts...>::value,
                  "castle::get<T>: T is not an alternative");
    return castle::get<detail::variant_type_index<U, Ts...>::value>(value);
}

/** @brief Returns the active const alternative by type. */
template <typename T, typename... Ts>
CASTLE_CONSTEXPR CASTLE_CONST T&
get(CASTLE_CONST variant<Ts...>& value) CASTLE_NOEXCEPT
{
    using U = meta::remove_cv_t<T>;
    static_assert(detail::variant_has_type<U, Ts...>::value,
                  "castle::get<T>: T is not an alternative");
    return castle::get<detail::variant_type_index<U, Ts...>::value>(value);
}

/** @brief Returns the active alternative by type as an rvalue reference. */
template <typename T, typename... Ts>
CASTLE_CONSTEXPR T&& get(variant<Ts...>&& value) CASTLE_NOEXCEPT
{
    return CASTLE_MOVE(castle::get<T>(value));
}

/** @brief Returns the active const alternative by type as an rvalue reference. */
template <typename T, typename... Ts>
CASTLE_CONSTEXPR CASTLE_CONST T&&
get(CASTLE_CONST variant<Ts...>&& value) CASTLE_NOEXCEPT
{
    return CASTLE_MOVE(castle::get<T>(value));
}




/**
 * @brief Returns a pointer to the active alternative by index, or `nullptr`.
 * @tparam Index Alternative index to retrieve.
 * @tparam Ts Variant alternative types.
 * @param value Variant pointer to inspect.
 * @return Pointer to the active alternative, or `nullptr` on mismatch or null input.
 */
template <size_type Index, typename... Ts>
CASTLE_NODISCARD CASTLE_CONSTEXPR variant_alternative_t<Index, variant<Ts...>>*
get_if(variant<Ts...>* value) CASTLE_NOEXCEPT
{
    static_assert(Index < sizeof...(Ts),
                  "castle::get_if<Index>: index out of range");
    // LCOV_EXCL_BR_START
    return (value != nullptr && value->index() == Index)
           ? &value->template get_value<Index>()
           : nullptr;
    // LCOV_EXCL_BR_END
}

/** @brief Returns a const pointer to the active alternative by index, or `nullptr`. */
template <size_type Index, typename... Ts>
CASTLE_NODISCARD CASTLE_CONSTEXPR CASTLE_CONST variant_alternative_t<Index, variant<Ts...>>*
get_if(CASTLE_CONST variant<Ts...>* value) CASTLE_NOEXCEPT
{
    static_assert(Index < sizeof...(Ts),
                  "castle::get_if<Index>: index out of range");
    return (value != nullptr && value->index() == Index)
           ? &value->template get_value<Index>()
           : nullptr;
}

/** @brief Returns a pointer to the active alternative by type, or `nullptr`. */
template <typename T, typename... Ts>
CASTLE_NODISCARD CASTLE_CONSTEXPR T*
get_if(variant<Ts...>* value) CASTLE_NOEXCEPT
{
    using U = meta::remove_cv_t<T>;
    static_assert(detail::variant_has_type<U, Ts...>::value,
                  "castle::get_if<T>: T is not an alternative");
    return (value != nullptr && holds_alternative<U>(*value))
           ? &castle::get<U>(*value)
           : nullptr;
}

/** @brief Returns a const pointer to the active alternative by type, or `nullptr`. */
template <typename T, typename... Ts>
CASTLE_NODISCARD CASTLE_CONSTEXPR CASTLE_CONST T*
get_if(CASTLE_CONST variant<Ts...>* value) CASTLE_NOEXCEPT
{
    using U = meta::remove_cv_t<T>;
    static_assert(detail::variant_has_type<U, Ts...>::value,
                  "castle::get_if<T>: T is not an alternative");
    return (value != nullptr && holds_alternative<U>(*value))
           ? &castle::get<U>(*value)
           : nullptr;
}








/**
 * @brief Visits the active alternative with a callable object.
 * @tparam Visitor Callable type.
 * @tparam Ts Variant alternative types.
 * @param visitor Callable invoked with the active alternative.
 * @param value Variant to visit.
 * @return The visitor's return value.
 */
template <typename Visitor, typename... Ts>
meta::invoke_result_t<Visitor&&, detail::variant_type_at_t<0U, Ts...>&>
visit(Visitor&& visitor, variant<Ts...>& value)
{
    using return_type = meta::invoke_result_t<Visitor&&, detail::variant_type_at_t<0U, Ts...>&>;
    static_assert(detail::variant_visit_invocable<Visitor&&, Ts&...>::value,
                  "castle::visit visitor must support every alternative");
    return variant<Ts...>::template visit_impl<Visitor&&, variant<Ts...>&, return_type, 0U>(
        CASTLE_FORWARD<Visitor>(visitor), value
    );
}

/** @brief Visits the active const alternative with a callable object. */
template <typename Visitor, typename... Ts>
meta::invoke_result_t<Visitor&&, CASTLE_CONST detail::variant_type_at_t<0U, Ts...>&>
visit(Visitor&& visitor, CASTLE_CONST variant<Ts...>& value)
{
    using return_type = meta::invoke_result_t<Visitor&&, CASTLE_CONST detail::variant_type_at_t<0U, Ts...>&>;
    static_assert(detail::variant_visit_invocable<Visitor&&, CASTLE_CONST Ts&...>::value,
                  "castle::visit visitor must support every alternative");
    return variant<Ts...>::template visit_impl<Visitor&&, CASTLE_CONST variant<Ts...>&, return_type, 0U>(
        CASTLE_FORWARD<Visitor>(visitor), value
    );
}

/** @brief Visits the active alternative of an rvalue variant. */
template <typename Visitor, typename... Ts>
meta::invoke_result_t<Visitor&&, detail::variant_type_at_t<0U, Ts...>&&>
visit(Visitor&& visitor, variant<Ts...>&& value)
{
    using return_type = meta::invoke_result_t<Visitor&&, detail::variant_type_at_t<0U, Ts...>&&>;
    static_assert(detail::variant_visit_invocable<Visitor&&, Ts&&...>::value,
                  "castle::visit visitor must support every alternative");
    return variant<Ts...>::template visit_impl<Visitor&&, variant<Ts...>&&, return_type, 0U>(
        CASTLE_FORWARD<Visitor>(visitor), CASTLE_MOVE(value)
    );
}

/** @brief Visits the active const alternative of an rvalue variant. */
template <typename Visitor, typename... Ts>
meta::invoke_result_t<Visitor&&, CASTLE_CONST detail::variant_type_at_t<0U, Ts...>&&>
visit(Visitor&& visitor, CASTLE_CONST variant<Ts...>&& value)
{
    using return_type = meta::invoke_result_t<Visitor&&, CASTLE_CONST detail::variant_type_at_t<0U, Ts...>&&>;
    static_assert(detail::variant_visit_invocable<Visitor&&, CASTLE_CONST Ts&&...>::value,
                  "castle::visit visitor must support every alternative");
    return variant<Ts...>::template visit_impl<Visitor&&, CASTLE_CONST variant<Ts...>&&, return_type, 0U>(
        CASTLE_FORWARD<Visitor>(visitor), CASTLE_MOVE(value)
    );
}




/** @brief Swaps two variants. */
template <typename... Ts>
void swap(variant<Ts...>& lhs, variant<Ts...>& rhs)
    CASTLE_NOEXCEPT(CASTLE_NOEXCEPT(lhs.swap(rhs)))
{
    lhs.swap(rhs);
}




/**
 * @brief Creates a single-alternative variant in place.
 * @tparam T Alternative type to construct.
 * @tparam Args Constructor argument types.
 * @param args Arguments forwarded to `T`'s constructor.
 * @return `variant<T>` containing the constructed value.
 */
template <typename T, typename... Args,
          meta::enable_if_t<meta::is_constructible<T, Args&&...>::value, int> = 0>
variant<T> make_variant(Args&&... args)
    CASTLE_NOEXCEPT(meta::is_nothrow_constructible<T, Args&&...>::value)
{
    return variant<T>(meta::in_place_type<T>, CASTLE_FORWARD<Args>(args)...);
}

} 

#endif 
