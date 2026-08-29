#pragma once

#include "castle/types/traits.h"

using namespace castle::types;

namespace castle
{
namespace design_patterns
{

//*****************************************************************
/// @brief Visitor variadic case.
/// Recursively inherits from visitor<T1> and visitor<Types...>,
/// pulling all visit() overloads into scope.
/// Usage: using ShapeVisitor = visitor<Class1&, Class2&, const Class3&>;
/// @tparam T1     First visit parameter type.
/// @tparam Types  Remaining visit parameter types.
//*****************************************************************
template <typename T1, typename... Types>
class visitor
    : public visitor<T1>
    , public visitor<Types...>
{
    static_assert(has_unique_types_v<T1, Types...>, "Visitor types must be unique.");

public:
    using visitor<T1>::visit;
    using visitor<Types...>::visit;
};

//*****************************************************************
/// @brief Visitor base case for a single type.
/// Provides one pure-virtual visit(T1) overload.
/// @tparam T1 The visit parameter type (value, ref, or const ref).
//*****************************************************************
template <typename T1>
class visitor<T1>
{
public:
    virtual ~visitor() = default;
    virtual void visit(T1) = 0;
};

//*****************************************************************
/// @brief Visitable variadic case.
/// Recursively inherits from visitable<T1> and visitable<Types...>,
/// pulling all accept() overloads into scope.
/// Usage: class Shape : public visitable<ShapeVisitor> {};
/// @tparam T1     First visitor type.
/// @tparam Types  Remaining visitor types.
//*****************************************************************
template <typename T1, typename... Types>
class visitable
    : public visitable<T1>
    , public visitable<Types...>
{
    static_assert(has_unique_types_v<T1, Types...>, "Visitable types must be unique.");

public:
    using visitable<T1>::accept;
    using visitable<Types...>::accept;
};

//*****************************************************************
/// @brief Visitable base case for a single visitor type.
/// Provides one pure-virtual accept(T1&) overload.
/// @tparam T1 The visitor type.
//*****************************************************************
template <typename T1>
class visitable<T1>
{
public:
    virtual ~visitable() = default;
    virtual void accept(T1&) = 0;
};

} // namespace design_patterns
} // namespace castle
