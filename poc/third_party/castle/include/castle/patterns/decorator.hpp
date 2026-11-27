// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file decorator.hpp
 * @brief Template Decorator: wraps a callable in outer layers that add behavior without changing its call syntax.
 *
 * A *decorator* is any callable invocable as `decorator(next, args...)`. It receives the wrapped callable as `next`
 * and the caller's arguments, and decides what happens around the call: run code before or after `next(args...)`,
 * change the arguments, transform or replace the result, call `next` several times (retry), or not at all (guard,
 * cache hit). This is the Python decorator model written with templates:
 *
 * @code
 * # Python                      // CASTLE
 * @trace                        auto f = castle::patterns::decorate(target, trace, retry);
 * @retry                        f(args...); // trace -> retry -> target
 * def target(...): ...
 * @endcode
 *
 * `decorated<Target, Decorator>` owns the target and the decorator by value; `decorate()` deduces it and stacks
 * several decorators, the first one being the outermost layer. The result is itself a callable, so it can be decorated
 * again, passed to any component taking a callable, or type-erased into a `castle::callbacks::function` large enough to
 * hold it. Targets may be function pointers, lambdas, functors, `castle::callbacks::function`, delegates, execution
 * policies, or other decorated callables.
 *
 * GoF mapping: the call signature is the Component interface (static duck typing, no base class), the target is the
 * ConcreteComponent, `decorated` is the Decorator that holds and forwards to the wrapped component, and each decorator
 * callable is a ConcreteDecorator. No inheritance, virtual functions, RTTI, heap allocation, or global state is used;
 * every layer is resolved at compile time and can be inlined.
 *
 * @code
 * #include "castle/patterns/decorator.hpp"
 * #include "castle/callbacks/function.hpp"
 *
 * castle::callbacks::function<int(int)> scale = [](int value) noexcept { return value * 10; };
 *
 * auto clamped = castle::patterns::decorate(
 *     scale,
 *     [](auto& next, int value) noexcept
 *     {
 *         int const result = next(value);
 *         return result > 100 ? 100 : result;
 *     });
 *
 * int const out = clamped(42); // 100
 * @endcode
 */
#ifndef CASTLE_PATTERNS_DECORATOR_HPP
#define CASTLE_PATTERNS_DECORATOR_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/utility/forward.hpp"

namespace castle
{
namespace patterns
{

/**
 * @brief A callable target wrapped by one decorator layer.
 *
 * @tparam Target Wrapped callable; passed to the decorator as `next` (`Target&`).
 * @tparam Decorator Callable invocable as `decorator(next, args...)`.
 *
 * Calling a `decorated` object with `args...` calls `decorator(target, args...)` and returns its result unchanged,
 * including references. Both callables are stored by value; copying the wrapper copies both, so each copy has its own
 * decorator state.
 */
template <typename Target, typename Decorator>
class decorated
{
public:
    using target_type = Target;
    using decorator_type = Decorator;

    /**
     * @brief Constructs the wrapper from its target and decorator.
     * @param target Callable to wrap.
     * @param decorator Outer layer invoked as `decorator(target, args...)`.
     */
    template <typename TargetArg, typename DecoratorArg>
    CASTLE_CONSTEXPR decorated(TargetArg&& target, DecoratorArg&& decorator)
        : target_(CASTLE_FORWARD<TargetArg>(target)),
          decorator_(CASTLE_FORWARD<DecoratorArg>(decorator))
    {
    }

    /**
     * @brief Calls the decorator with the target as `next`.
     * @param args Arguments forwarded unchanged to the decorator.
     * @return Whatever the decorator returns.
     */
    template <typename... Args>
    CASTLE_CONSTEXPR decltype(auto) operator()(Args&&... args)
    {
        static_assert(meta::is_invocable<Decorator&, Target&, Args&&...>::value,
                      "Decorator must be callable as decorator(next, args...)");

        return decorator_(target_, CASTLE_FORWARD<Args>(args)...);
    }

    /** @brief Returns the wrapped callable. */
    CASTLE_CONSTEXPR Target& target() CASTLE_NOEXCEPT
    {
        return target_;
    }

    /** @brief Returns the decorator. */
    CASTLE_CONSTEXPR Decorator& decorator() CASTLE_NOEXCEPT
    {
        return decorator_;
    }

private:
    Target target_;
    Decorator decorator_;
};

/**
 * @brief Wraps a callable in one decorator.
 * @param target Callable to wrap; decay-copied (or moved) into the result.
 * @param decorator Callable invoked as `decorator(next, args...)`; decay-copied (or moved) into the result.
 * @return `decorated<decay_t<Target>, decay_t<Decorator>>`.
 */
template <typename Target, typename Decorator>
CASTLE_CONSTEXPR decorated<meta::decay_t<Target>, meta::decay_t<Decorator>>
decorate(Target&& target, Decorator&& decorator)
{
    return decorated<meta::decay_t<Target>, meta::decay_t<Decorator>>(CASTLE_FORWARD<Target>(target),
                                                                     CASTLE_FORWARD<Decorator>(decorator));
}

/**
 * @brief Wraps a callable in several decorators; the first decorator is the outermost layer.
 * @param target Callable to wrap.
 * @param outer Outermost decorator: it runs first and its `next` is the rest of the stack.
 * @param second Next decorator.
 * @param inner Remaining decorators, outer to inner; the last one calls the target directly.
 * @return Nested `decorated` objects equivalent to `decorate(decorate(target, second, inner...), outer)`.
 */
template <typename Target, typename Outer, typename Second, typename... Inner>
CASTLE_CONSTEXPR auto decorate(Target&& target, Outer&& outer, Second&& second, Inner&&... inner)
{
    return decorate(decorate(CASTLE_FORWARD<Target>(target),
                             CASTLE_FORWARD<Second>(second),
                             CASTLE_FORWARD<Inner>(inner)...),
                    CASTLE_FORWARD<Outer>(outer));
}

} // namespace patterns
} // namespace castle

#endif // CASTLE_PATTERNS_DECORATOR_HPP
