// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file object_graph.hpp
 * @brief Compile-time composition root: one object owns a fixed set of objects, wires them, and orders their lifetimes.
 *
 * An application usually builds its context by hand: a struct of members, each constructed
 * from references to earlier members, plus `init()` / `deinit()` functions that must call
 * every member in the right order and undo a half-finished start-up. `object_graph` writes
 * that code from one type declaration:
 *
 * - every object is a member of the graph, so storage is static or automatic, never heap;
 * - each object lists its relations: `init_with<>` (read-only construction input),
 *   `uses<>` (aggregation, a shared reference) and `owns<>` (composition, an exclusive part);
 * - the graph constructs objects in declaration order and passes the referenced objects to
 *   each constructor, and C++ destroys them in reverse order, so every referenced object
 *   outlives the objects that refer to it;
 * - the compiler rejects a relation to an object declared later or missing from the graph,
 *   and a part owned by two objects;
 * - `init()` calls each object's optional `init()` hook in declaration order and rolls back
 *   on failure; `deinit()` calls the optional `deinit()` hooks in reverse order.
 *
 * @code
 * #include "castle/patterns/object_graph.hpp"
 *
 * namespace dp = castle::patterns;
 *
 * struct uart_config { uint32_t baud = 115200U; };
 * struct uart_node   { explicit uart_node(const uart_config& cfg); castle::status init(); void deinit(); };
 * struct data_pool   { };
 * struct ubx_parser  { explicit ubx_parser(data_pool& pool); };
 *
 * using app_context = dp::object_graph<
 *     uart_config,                                   // plain type: default-constructed, no relations
 *     data_pool,
 *     dp::object<uart_node,  dp::init_with<uart_config>>,
 *     dp::object<ubx_parser, dp::uses<data_pool>>>;
 *
 * app_context context;                               // constructs and wires every object
 * castle::status started = context.init();           // uart_node::init(); rollback on failure
 * context.get<ubx_parser>();                         // typed access
 * @endcode
 */
#ifndef CASTLE_PATTERNS_OBJECT_GRAPH_HPP
#define CASTLE_PATTERNS_OBJECT_GRAPH_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/status.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/patterns/composite.hpp"
#include "castle/utility/integral_sequence.hpp"
#include "castle/utility/tuple.hpp"

#include <stdint.h>

namespace castle
{
namespace patterns
{

template <typename T, typename... Relations>
struct object;

namespace object_graph_detail
{

/** @brief Compile-time list of types. */
template <typename... Ts>
struct type_list
{
};

/** @brief Concatenates `type_list`s into one `type_list`. */
template <typename... Lists>
struct concat
{
    using type = type_list<>;
};

template <typename... Ts>
struct concat<type_list<Ts...> >
{
    using type = type_list<Ts...>;
};

template <typename... Ts, typename... Us, typename... Rest>
struct concat<type_list<Ts...>, type_list<Us...>, Rest...> : concat<type_list<Ts..., Us...>, Rest...>
{
};

/** @brief How a relation passes the referenced object to the constructor. */
enum class access : uint8_t
{
    read_only, /**< `init_with<>`: `const T&`, construction input. */
    shared,    /**< `uses<>`: `T&`, aggregation. */
    exclusive  /**< `owns<>`: `T&`, composition; one owner per object. */
};

/** @brief One constructor argument: the referenced object type and how it is passed. */
template <access Kind, typename T>
struct dependency
{
    using type = T;
    using reference = meta::conditional_t<Kind == access::read_only, CASTLE_CONST T&, T&>;
};

/** @brief Marker base of every relation type. */
struct relation_base
{
};

/** @brief Common implementation of `init_with<>`, `uses<>` and `owns<>`. */
template <access Kind, typename... Ts>
struct relation : relation_base
{
    static_assert(sizeof...(Ts) > 0U, "object_graph: a relation must name at least one object");

    using dependencies = type_list<dependency<Kind, Ts>...>;
};

/** @brief Maps a graph entry to its `object<>` description: a plain `T` becomes `object<T>`. */
template <typename Entry>
struct as_object
{
    using type = object<Entry>;
};

template <typename T, typename... Relations>
struct as_object<object<T, Relations...> >
{
    using type = object<T, Relations...>;
};

template <typename Entry>
using as_object_t = typename as_object<Entry>::type;

template <typename Entry>
using value_t = typename as_object_t<Entry>::value_type;

/** @brief Reports, with an error naming `T` in the instantiation trace, when `T` has more than one owner. */
template <typename T, typename AllDependencies>
struct single_owner_check;

template <typename T, typename... Dependencies>
struct single_owner_check<T, type_list<Dependencies...> >
{
    static_assert(meta::tuple_type_count<dependency<access::exclusive, T>, Dependencies...>::value <= 1U,
                  "object_graph: an object can be owned (owns<>) by at most one other object");

    static CASTLE_CONSTEXPR bool value = true;
};

/**
 * @brief Holds the object at position `Index` and constructs it from its dependencies.
 * @note `Storage` derives from every slot in index order, so earlier slots are fully
 *       constructed when this slot's constructor reads them.
 */
template <size_type Index, typename Object>
struct slot
{
    using value_type = typename Object::value_type;

    template <typename Storage>
    explicit slot(Storage& storage)
        : slot(storage, typename Object::dependencies{})
    {
    }

    template <typename Storage, typename... Dependencies>
    slot(Storage& storage, type_list<Dependencies...>)
        : value(Storage::template resolve<Index, Dependencies>(storage)...)
    {
        static_assert(meta::is_constructible<value_type, typename Dependencies::reference...>::value,
                      "object_graph: object must be constructible from its relations, in declaration order "
                      "(init_with<T> passes const T&, uses<T> and owns<T> pass T&)");
    }

    slot(CASTLE_CONST slot&) CASTLE_DELETE;
    slot& operator=(CASTLE_CONST slot&) CASTLE_DELETE;

    value_type value;
};

template <typename Sequence, typename... Objects>
struct storage;

/** @brief Every object of the graph, as base subobjects constructed in declaration order. */
template <size_type... Indices, typename... Objects>
struct storage<sequence::index_sequence<Indices...>, Objects...> : slot<Indices, Objects>...
{
    using all_dependencies = typename concat<typename Objects::dependencies...>::type;

    storage()
        : slot<Indices, Objects>(*this)...
    {
    }

    storage(CASTLE_CONST storage&) CASTLE_DELETE;
    storage& operator=(CASTLE_CONST storage&) CASTLE_DELETE;

    template <typename T>
    static CASTLE_CONSTEXPR bool contains() CASTLE_NOEXCEPT
    {
        return meta::pack_contains<T, typename Objects::value_type...>::value;
    }

    template <typename T>
    static CASTLE_CONSTEXPR size_type index_of() CASTLE_NOEXCEPT
    {
        return meta::tuple_type_index<T, typename Objects::value_type...>::value;
    }

    template <size_type Index>
    using slot_at = slot<Index, tuple_element_t<Index, castle::tuple<Objects...> > >;

    // Static, so no member function runs on the storage before all its bases are constructed.
    template <typename T>
    static T& get(storage& self) CASTLE_NOEXCEPT
    {
        static_assert(contains<T>(), "object_graph: type is not an object of this graph");
        return static_cast<slot_at<index_of<T>()>&>(self).value;
    }

    template <typename T>
    static CASTLE_CONST T& get(CASTLE_CONST storage& self) CASTLE_NOEXCEPT
    {
        static_assert(contains<T>(), "object_graph: type is not an object of this graph");
        return static_cast<CASTLE_CONST slot_at<index_of<T>()>&>(self).value;
    }

    /** @brief Returns the argument passed for `Dependency` to the constructor of object `User`. */
    template <size_type User, typename Dependency>
    static typename Dependency::reference resolve(storage& self) CASTLE_NOEXCEPT
    {
        using type = typename Dependency::type;
        static_assert(contains<type>(), "object_graph: a relation names a type that is not an object of this graph");
        static_assert(!contains<type>() || (index_of<type>() < User),
                      "object_graph: a related object must be declared before the object that refers to it, "
                      "so it is constructed first and destroyed last");
        return get<type>(self);
    }
};

template <typename T, typename = void>
struct has_init : meta::false_type
{
};

template <typename T>
struct has_init<T, meta::void_t<decltype(meta::declval<T&>().init())> > : meta::true_type
{
};

template <typename T, typename = void>
struct has_deinit : meta::false_type
{
};

template <typename T>
struct has_deinit<T, meta::void_t<decltype(meta::declval<T&>().deinit())> > : meta::true_type
{
};

template <typename Result>
struct is_hook_result
    : meta::bool_constant<meta::is_void<Result>::value || meta::is_same<meta::decay_t<Result>, status>::value>
{
};

/** @brief Calls `object.init()` when it exists; `void` hooks and missing hooks report `status::ok`. */
template <typename T>
status call_init(T& object)
{
    CASTLE_IF_CONSTEXPR (has_init<T>::value)
    {
        using result_type = decltype(object.init());
        static_assert(is_hook_result<result_type>::value, "object_graph: init() must return void or castle::status");

        CASTLE_IF_CONSTEXPR (meta::is_void<result_type>::value)
        {
            object.init();
            return status::ok;
        }
        else
        {
            return object.init();
        }
    }
    else
    {
        return status::ok;
    }
}

/** @brief Calls `object.deinit()` when it exists; `void` hooks and missing hooks report `status::ok`. */
template <typename T>
status call_deinit(T& object)
{
    CASTLE_IF_CONSTEXPR (has_deinit<T>::value)
    {
        using result_type = decltype(object.deinit());
        static_assert(is_hook_result<result_type>::value, "object_graph: deinit() must return void or castle::status");

        CASTLE_IF_CONSTEXPR (meta::is_void<result_type>::value)
        {
            object.deinit();
            return status::ok;
        }
        else
        {
            return object.deinit();
        }
    }
    else
    {
        return status::ok;
    }
}

} // namespace object_graph_detail

/**
 * @brief Read-only construction input: each `T` is passed to the constructor as `const T&`.
 * @tparam Ts Objects of the same graph, declared before the object that names them.
 * @note Use it for configuration the object reads while it is constructed.
 */
template <typename... Ts>
struct init_with : object_graph_detail::relation<object_graph_detail::access::read_only, Ts...>
{
};

/**
 * @brief Aggregation: each `T` is passed to the constructor as `T&` and may be shared.
 * @tparam Ts Objects of the same graph, declared before the object that names them.
 * @note Any number of objects may use the same object.
 */
template <typename... Ts>
struct uses : object_graph_detail::relation<object_graph_detail::access::shared, Ts...>
{
};

/**
 * @brief Composition: each `T` is passed to the constructor as `T&` and is a part of this object.
 * @tparam Ts Objects of the same graph, declared before the object that names them.
 * @note A part has exactly one owner in the graph; other objects may still `uses<>` it.
 */
template <typename... Ts>
struct owns : object_graph_detail::relation<object_graph_detail::access::exclusive, Ts...>
{
};

/**
 * @brief Describes one graph object: its type and its relations to earlier objects.
 * @tparam T Object type. Must be a non-cv, non-array object type, unique within the graph.
 * @tparam Relations Any sequence of `init_with<>`, `uses<>` and `owns<>`.
 *
 * `T` is constructed with one argument per related object, in the order the relations and
 * their types are written. `object<T>` without relations is value-initialized; a plain `T`
 * in the graph means `object<T>`.
 */
template <typename T, typename... Relations>
struct object
{
    static_assert(meta::is_object<T>::value && !meta::is_array<T>::value,
                  "object_graph: object type must be a non-array object type");
    static_assert(meta::is_same<T, meta::remove_cv_t<T> >::value,
                  "object_graph: object type must not be cv-qualified; use init_with<> for read-only access");
    static_assert((meta::is_base_of<object_graph_detail::relation_base, Relations>::value && ...),
                  "object_graph: object relations must be init_with<>, uses<> or owns<>");

    using value_type = T;
    using dependencies = typename object_graph_detail::concat<typename Relations::dependencies...>::type;
};

/**
 * @brief Owns a fixed set of objects, constructs them with their relations, and orders their lifecycle.
 *
 * @tparam Entries Graph objects in lifetime order: each entry is a plain type `T` or an
 *         `object<T, Relations...>`. Earlier objects are constructed first and destroyed last.
 *
 * Construction wires the graph and cannot fail. Work that can fail belongs in the optional
 * hooks `init()` and `deinit()` (returning `void` or `castle::status`), driven by `init()` and
 * `deinit()` of the graph. Objects without a hook are skipped.
 *
 * @note The graph is neither copyable nor movable: objects hold references into it.
 * @note No synchronization is performed; call `init()` and `deinit()` from one context.
 */
template <typename... Entries>
class object_graph
{
    static_assert(sizeof...(Entries) > 0U, "object_graph requires at least one object");
    static_assert(meta::has_unique_types<object_graph_detail::value_t<Entries>...>::value,
                  "object_graph: every object type must appear only once");

    using storage_type = object_graph_detail::storage<sequence::index_sequence_for<Entries...>,
                                                      object_graph_detail::as_object_t<Entries>...>;

    static_assert((object_graph_detail::single_owner_check<object_graph_detail::value_t<Entries>,
                                                           typename storage_type::all_dependencies>::value &&
                   ...),
                  "object_graph: an object can be owned (owns<>) by at most one other object");

public:
    using size_type = castle::size_type;

    /** @brief Non-owning composite of every object, in declaration order. */
    using objects_type = composite<object_graph_detail::value_t<Entries>&...>;

    /** @brief Const non-owning composite of every object, in declaration order. */
    using const_objects_type = composite<CASTLE_CONST object_graph_detail::value_t<Entries>&...>;

    /** @brief Number of objects in the graph. */
    static CASTLE_CONSTEXPR size_type object_count = sizeof...(Entries);

    /** @brief Constructs every object in declaration order, passing each one its related objects. */
    object_graph() CASTLE_DEFAULT;

    object_graph(CASTLE_CONST object_graph&) CASTLE_DELETE;
    object_graph& operator=(CASTLE_CONST object_graph&) CASTLE_DELETE;
    object_graph(object_graph&&) CASTLE_DELETE;
    object_graph& operator=(object_graph&&) CASTLE_DELETE;

    /**
     * @brief Calls `deinit()` when the graph is still initialized, then destroys every object in reverse order.
     * @note The `deinit()` result is discarded; call `deinit()` explicitly to observe it.
     */
    ~object_graph()
    {
        if (initialized_)
        {
            static_cast<void>(deinit());
        }
    }

    /** @brief Returns the number of objects in the graph. */
    static CASTLE_CONSTEXPR size_type size() CASTLE_NOEXCEPT
    {
        return object_count;
    }

    /** @brief Reports whether `T` is an object of this graph. */
    template <typename T>
    static CASTLE_CONSTEXPR bool contains() CASTLE_NOEXCEPT
    {
        return storage_type::template contains<T>();
    }

    /**
     * @brief Returns the object of type `T`.
     * @tparam T One of the graph's object types; any other type does not compile.
     */
    template <typename T>
    CASTLE_NODISCARD T& get() CASTLE_NOEXCEPT
    {
        return storage_type::template get<T>(storage_);
    }

    /** @brief Const overload of `get()`. */
    template <typename T>
    CASTLE_NODISCARD CASTLE_CONST T& get() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return storage_type::template get<T>(storage_);
    }

    /**
     * @brief Returns a composite that refers to every object, for custom fan-out operations.
     * @note Use `for_each_child()` to visit exactly the graph objects; `for_each()` would also
     *       expand objects that are themselves composites.
     */
    CASTLE_NODISCARD objects_type objects() CASTLE_NOEXCEPT
    {
        return objects_type(get<object_graph_detail::value_t<Entries> >()...);
    }

    /** @brief Const overload of `objects()`. */
    CASTLE_NODISCARD const_objects_type objects() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return const_objects_type(get<object_graph_detail::value_t<Entries> >()...);
    }

    /**
     * @brief Calls every `init()` hook in declaration order, stopping at the first failure.
     * @return `status::ok` when every hook succeeded; `status::already_exists` when the graph is
     *         already initialized; otherwise the failing hook's status.
     * @note On failure, the `deinit()` hooks of the objects already initialized run in reverse
     *       order and the graph stays uninitialized.
     */
    status init()
    {
        if (initialized_)
        {
            return status::already_exists;
        }

        status result = status::ok;
        size_type ready = 0U;
        // Branch outcomes below depend on the hooks of each object type, so they are excluded per instantiation.
        objects().for_each_child([&result, &ready](auto& item)
        {
            if (succeeded(result)) // LCOV_EXCL_BR_LINE
            {
                result = object_graph_detail::call_init(item);
                if (succeeded(result)) // LCOV_EXCL_BR_LINE
                {
                    ++ready;
                }
            }
        });

        if (!succeeded(result)) // LCOV_EXCL_BR_LINE
        {
            size_type index = object_count;
            objects().for_each_child(reverse_order, [&index, ready](auto& item)
            {
                --index;
                if (index < ready) // LCOV_EXCL_BR_LINE
                {
                    static_cast<void>(object_graph_detail::call_deinit(item));
                }
            });
            return result;
        }

        initialized_ = true;
        return status::ok;
    }

    /**
     * @brief Calls every `deinit()` hook in reverse declaration order.
     * @return The first failure reported by a hook, `status::not_configured` when the graph is
     *         not initialized, or `status::ok`.
     * @note Every hook runs even after a failure; the graph is uninitialized afterwards.
     */
    status deinit()
    {
        if (!initialized_)
        {
            return status::not_configured;
        }

        initialized_ = false;
        return objects().for_each_child(reverse_order, [](auto& item)
        {
            return object_graph_detail::call_deinit(item);
        });
    }

    /** @brief Reports whether `init()` succeeded and `deinit()` has not run since. */
    CASTLE_NODISCARD bool is_initialized() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return initialized_;
    }

private:
    storage_type storage_;
    bool initialized_ = false;
};

} // namespace patterns
} // namespace castle

#endif // CASTLE_PATTERNS_OBJECT_GRAPH_HPP
