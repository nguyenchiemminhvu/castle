#ifndef CASTLE_UTILITY_VARIANT_H
#define CASTLE_UTILITY_VARIANT_H

#include "castle/core/compiler.h"
#include "castle/core/error_handler.h"
#include "castle/core/types.h"
#include "castle/core/type_ranges.h"
#include "castle/core/traits.h"
#include "castle/memory/construct.h"
#include "castle/memory/destroy.h"
#include "castle/memory/alignment.h"
#include "castle/utility/forward.h"
#include "castle/utility/move.h"
#include "castle/utility/swap.h"

namespace castle
{
// ============================================================================
// in_place_index_t / in_place_index
//
// Tag used to select a variant alternative by compile-time index.
// ============================================================================
template <size_type Index>
struct in_place_index_t
{
    explicit CASTLE_CONSTEXPR in_place_index_t() CASTLE_NOEXCEPT {}
};

template <size_type Index>
inline CASTLE_CONSTEXPR in_place_index_t<Index> in_place_index{};

// ============================================================================
// variant_npos
//
// Sentinel returned by variant::index() while the variant contains no active
// alternative. A variant normally becomes valueless only if an underlying
// constructor/operation throws while exceptions are enabled.
// ============================================================================
static CASTLE_CONSTEXPR size_type variant_npos = castle::numeric_limits<size_type>().max();

namespace detail
{

// ============================================================================
// variant type-list helpers
// ============================================================================
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

} // namespace detail

// ============================================================================
// variant
//
// Fixed-size discriminated union for embedded systems.
//
// The implementation intentionally favours explicit lifetime management and
// predictable storage over the more elaborate exception machinery of STL
// variants. The normal embedded configuration should use non-throwing types.
// ============================================================================
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

    // ------------------------------------------------------------------------
    // Construction.
    // ------------------------------------------------------------------------
    variant()
        CASTLE_NOEXCEPT(meta::is_nothrow_default_constructible<alternative_type<0U>>::value)
        : index_(variant_npos)
    {
        emplace<0U>();
    }


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
    variant(T&& value)
        CASTLE_NOEXCEPT(meta::is_nothrow_constructible<U, T&&>::value)
        : index_(variant_npos)
    {
        emplace<U>(CASTLE_FORWARD<T>(value));
    }

    variant(CASTLE_CONST variant& other)
        CASTLE_NOEXCEPT(meta::conjunction<meta::is_nothrow_copy_constructible<Ts>...>::value)
        : index_(variant_npos)
    {
        copy_from(other);
    }

    variant(variant&& other)
        CASTLE_NOEXCEPT(meta::conjunction<meta::is_nothrow_move_constructible<Ts>...>::value)
        : index_(variant_npos)
    {
        move_from<0U>(other);
    }

    ~variant() CASTLE_NOEXCEPT
    {
        reset();
    }

    // ------------------------------------------------------------------------
    // Assignment.
    // ------------------------------------------------------------------------
    variant& operator=(CASTLE_CONST variant& other)
        CASTLE_NOEXCEPT(meta::conjunction<meta::is_nothrow_copy_constructible<Ts>...>::value)
    {
        if (this != &other)
        {
            reset();
            copy_from(other);
        }
        return *this;
    }

    variant& operator=(variant&& other)
        CASTLE_NOEXCEPT(meta::conjunction<meta::is_nothrow_move_constructible<Ts>...>::value)
    {
        if (this != &other)
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
    variant& operator=(T&& value)
        CASTLE_NOEXCEPT(meta::is_nothrow_constructible<U, T&&>::value)
    {
        emplace<U>(CASTLE_FORWARD<T>(value));
        return *this;
    }

    // ------------------------------------------------------------------------
    // State.
    // ------------------------------------------------------------------------
    CASTLE_NODISCARD CASTLE_CONSTEXPR size_type index() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return index_;
    }

    CASTLE_NODISCARD CASTLE_CONSTEXPR bool valueless_by_exception() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return index_ == variant_npos;
    }

    // Explicitly clear the active alternative. This is an embedded-friendly
    // extension; std::variant does not expose reset(), but deterministic state
    // machines often benefit from it.
    void reset() CASTLE_NOEXCEPT
    {
        if (index_ != variant_npos)
        {
            destroy_active<0U>();
            index_ = variant_npos;
        }
    }

    // ------------------------------------------------------------------------
    // In-place construction.
    // ------------------------------------------------------------------------
    template <size_type Index, typename... Args,
              meta::enable_if_t<
                  (Index < sizeof...(Ts)) &&
                  meta::is_constructible<alternative_type<Index>, Args&&...>::value,
                  int> = 0>
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
    U& emplace(Args&&... args)
        CASTLE_NOEXCEPT(meta::is_nothrow_constructible<U, Args&&...>::value)
    {
        return emplace<detail::variant_type_index<U, Ts...>::value>(
            CASTLE_FORWARD<Args>(args)...
        );
    }

    // ------------------------------------------------------------------------
    // Type support query.
    // ------------------------------------------------------------------------
    template <typename T>
    static CASTLE_CONSTEXPR bool is_supported_type() CASTLE_NOEXCEPT
    {
        using U = meta::remove_cv_t<T>;
        return detail::variant_has_type<U, Ts...>::value;
    }

    // ------------------------------------------------------------------------
    // Swap.
    // ------------------------------------------------------------------------
    void swap(variant& other)
        CASTLE_NOEXCEPT(meta::conjunction<meta::is_nothrow_move_constructible<Ts>...>::value)
    {
        if (this == &other)
        {
            return;
        }

        variant temporary(CASTLE_MOVE(*this));
        *this = CASTLE_MOVE(other);
        other = CASTLE_MOVE(temporary);
    }

    // Internal storage access used by castle::get()/castle::visit().
    template <size_type Index>
    alternative_type<Index>& get_value() & CASTLE_NOEXCEPT
    {
        return storage_.template get_reference<alternative_type<Index>>();
    }

    template <size_type Index>
    CASTLE_CONST alternative_type<Index>& get_value() CASTLE_CONST & CASTLE_NOEXCEPT
    {
        return storage_.template get_reference<alternative_type<Index>>();
    }

    template <size_type Index>
    alternative_type<Index>&& get_value() && CASTLE_NOEXCEPT
    {
        return CASTLE_MOVE(get_value<Index>());
    }

    template <size_type Index>
    CASTLE_CONST alternative_type<Index>&& get_value() CASTLE_CONST && CASTLE_NOEXCEPT
    {
        return CASTLE_MOVE(get_value<Index>());
    }

    // Internal runtime-dispatch helper used by castle::visit().
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

        CASTLE_IF_CONSTEXPR (Index + 1U < sizeof...(Ts))
        {
            return visit_impl<Visitor, Variant, Return, Index + 1U>(
                CASTLE_FORWARD<Visitor>(visitor),
                CASTLE_FORWARD<Variant>(value)
            );
        }

        CASTLE_ASSERT_FAIL(CASTLE_ERROR_GENERIC("castle::variant: invalid visit"));
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
        if (other.index_ != variant_npos)
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

// ============================================================================
// variant_alternative / variant_size
// ============================================================================
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

template <size_type Index, typename Variant>
using variant_alternative_t = typename variant_alternative<Index, Variant>::type;

template <typename Variant>
struct variant_size;

template <typename... Ts>
struct variant_size<variant<Ts...>>
    : integral_constant<size_type, sizeof...(Ts)>
{
};

template <typename Variant>
inline CASTLE_CONSTEXPR size_type variant_size_v = variant_size<Variant>::value;

// ============================================================================
// holds_alternative
// ============================================================================
template <typename T, typename... Ts>
CASTLE_NODISCARD CASTLE_CONSTEXPR bool
holds_alternative(CASTLE_CONST variant<Ts...>& value) CASTLE_NOEXCEPT
{
    using U = meta::remove_cv_t<T>;
    static_assert(detail::variant_has_type<U, Ts...>::value,
                  "castle::holds_alternative<T>: T is not an alternative");
    return value.index() == detail::variant_type_index<U, Ts...>::value;
}

template <size_type Index, typename... Ts>
CASTLE_NODISCARD CASTLE_CONSTEXPR bool
holds_alternative(CASTLE_CONST variant<Ts...>& value) CASTLE_NOEXCEPT
{
    static_assert(Index < sizeof...(Ts),
                  "castle::holds_alternative<Index>: index out of range");
    return value.index() == Index;
}

// ============================================================================
// get
// ============================================================================
template <size_type Index, typename... Ts>
CASTLE_CONSTEXPR variant_alternative_t<Index, variant<Ts...>>&
get(variant<Ts...>& value) CASTLE_NOEXCEPT
{
    static_assert(Index < sizeof...(Ts),
                  "castle::get<Index>: index out of range");
    CASTLE_ASSERT(value.index() == Index,
                  CASTLE_ERROR_GENERIC("castle::variant: incorrect alternative"));
    return value.template get_value<Index>();
}

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

template <size_type Index, typename... Ts>
CASTLE_CONSTEXPR variant_alternative_t<Index, variant<Ts...>>&&
get(variant<Ts...>&& value) CASTLE_NOEXCEPT
{
    return CASTLE_MOVE(castle::get<Index>(value));
}

template <size_type Index, typename... Ts>
CASTLE_CONSTEXPR CASTLE_CONST variant_alternative_t<Index, variant<Ts...>>&&
get(CASTLE_CONST variant<Ts...>&& value) CASTLE_NOEXCEPT
{
    return CASTLE_MOVE(castle::get<Index>(value));
}

template <typename T, typename... Ts>
CASTLE_CONSTEXPR T&
get(variant<Ts...>& value) CASTLE_NOEXCEPT
{
    using U = meta::remove_cv_t<T>;
    static_assert(detail::variant_has_type<U, Ts...>::value,
                  "castle::get<T>: T is not an alternative");
    return castle::get<detail::variant_type_index<U, Ts...>::value>(value);
}

template <typename T, typename... Ts>
CASTLE_CONSTEXPR CASTLE_CONST T&
get(CASTLE_CONST variant<Ts...>& value) CASTLE_NOEXCEPT
{
    using U = meta::remove_cv_t<T>;
    static_assert(detail::variant_has_type<U, Ts...>::value,
                  "castle::get<T>: T is not an alternative");
    return castle::get<detail::variant_type_index<U, Ts...>::value>(value);
}

template <typename T, typename... Ts>
CASTLE_CONSTEXPR T&& get(variant<Ts...>&& value) CASTLE_NOEXCEPT
{
    return CASTLE_MOVE(castle::get<T>(value));
}

template <typename T, typename... Ts>
CASTLE_CONSTEXPR CASTLE_CONST T&&
get(CASTLE_CONST variant<Ts...>&& value) CASTLE_NOEXCEPT
{
    return CASTLE_MOVE(castle::get<T>(value));
}

// ============================================================================
// get_if
// ============================================================================
template <size_type Index, typename... Ts>
CASTLE_NODISCARD CASTLE_CONSTEXPR variant_alternative_t<Index, variant<Ts...>>*
get_if(variant<Ts...>* value) CASTLE_NOEXCEPT
{
    static_assert(Index < sizeof...(Ts),
                  "castle::get_if<Index>: index out of range");
    return (value != nullptr && value->index() == Index)
           ? &value->template get_value<Index>()
           : nullptr;
}

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

// ============================================================================
// visit
//
// Runtime dispatch is expanded as a compile-time linear chain. For small
// embedded variants this gives a simple branch structure without tables,
// function pointers, virtual dispatch, or heap allocations.
// ============================================================================
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

// ============================================================================
// swap(variant, variant)
// ============================================================================
template <typename... Ts>
void swap(variant<Ts...>& lhs, variant<Ts...>& rhs)
    CASTLE_NOEXCEPT(CASTLE_NOEXCEPT(lhs.swap(rhs)))
{
    lhs.swap(rhs);
}

// ============================================================================
// make_variant
// ============================================================================
template <typename T, typename... Args,
          meta::enable_if_t<meta::is_constructible<T, Args&&...>::value, int> = 0>
variant<T> make_variant(Args&&... args)
    CASTLE_NOEXCEPT(meta::is_nothrow_constructible<T, Args&&...>::value)
{
    return variant<T>(meta::in_place_type<T>, CASTLE_FORWARD<Args>(args)...);
}

} // namespace castle

#endif // CASTLE_UTILITY_VARIANT_H
