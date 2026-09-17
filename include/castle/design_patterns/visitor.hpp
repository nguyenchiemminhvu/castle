// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @brief Variadic visitor and visitable base helpers for classic visitor-style dispatch.
 *
 * Use this header when several concrete types must share a visitor interface and the
 * application wants Castle's lightweight type-pack composition instead of STL utilities.
 * This implementation remains heap-free and RTTI-free, but it uses virtual functions for
 * `visit()` and `accept()` dispatch.
 *
 * @code
 * #include "castle/design_patterns/visitor.hpp"
 *
 * struct start_command;
 * struct stop_command;
 * using command_visitor = castle::design_patterns::visitor<start_command&, stop_command&>;
 * @endcode
 */
#ifndef CASTLE_DESIGN_PATTERNS_VISITOR_HPP
#define CASTLE_DESIGN_PATTERNS_VISITOR_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"

namespace castle
{
namespace design_patterns
{

/**
 * @brief Combines multiple `visit()` overload requirements into one interface.
 *
 * @tparam T1 First visit parameter type.
 * @tparam Types Remaining unique visit parameter types.
 *
 * @note Duplicate parameter types are rejected at compile time.
 */
template <typename T1, typename... Types>
class visitor
    : public visitor<T1>
    , public visitor<Types...>
{
    static_assert(meta::has_unique_types<T1, Types...>::value,
                  "Visitor types must be unique.");

public:
    using visitor<T1>::visit;
    using visitor<Types...>::visit;
};

/**
 * @brief Base visitor interface for one visit parameter type.
 *
 * @tparam T1 Visit parameter type.
 */
template <typename T1>
class visitor<T1>
{
public:
    /**
     * @brief Destroys the visitor interface.
     */
    virtual ~visitor() CASTLE_DEFAULT;

    /**
     * @brief Handles one visited object.
     *
     * @param value Visited object, typically passed by reference.
     */
    virtual void visit(T1 value) = 0;
};

/**
 * @brief Combines multiple `accept()` overload requirements into one interface.
 *
 * @tparam T1 First visitor type.
 * @tparam Types Remaining unique visitor types.
 */
template <typename T1, typename... Types>
class visitable
    : public visitable<T1>
    , public visitable<Types...>
{
    static_assert(meta::has_unique_types<T1, Types...>::value,
                  "Visitable types must be unique.");

public:
    using visitable<T1>::accept;
    using visitable<Types...>::accept;
};

/**
 * @brief Base visitable interface for one visitor type.
 *
 * @tparam T1 Visitor interface type.
 */
template <typename T1>
class visitable<T1>
{
public:
    /**
     * @brief Destroys the visitable interface.
     */
    virtual ~visitable() CASTLE_DEFAULT;

    /**
     * @brief Accepts a visitor and dispatches it to the concrete type.
     *
     * @param visitor Visitor instance that will receive the dispatch.
     */
    virtual void accept(T1& visitor) = 0;
};

} // namespace design_patterns
} // namespace castle

#endif // CASTLE_DESIGN_PATTERNS_VISITOR_HPP
