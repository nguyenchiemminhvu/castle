#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

namespace castle
{
namespace callbacks
{

// -----------------------------------------------------------------------------
// Signature-aware base interface. All callback variants below derive from
// this so the caller can hold a single i_function<R(Args...)>* regardless of
// how the target is bound. The primary template is left undefined so that a
// mistyped instantiation such as i_function<int, float> (i.e. non-signature
// form) fails to compile with a clear "incomplete type" diagnostic.
//
//   template <typename Signature>          class i_function;             // undefined
//   template <typename R, typename... Args>
//   class i_function<R(Args...)>;                                        // defined
//
// The convention throughout function.h is that every concrete callback type
// is parameterised by a function TYPE (R(Args...)) - the same style used by
// std::function - so R is discovered from the signature via partial
// specialisation. This keeps user-facing spellings uniform:
//
//   function<void(int)>                          cb1(&free_fn);
//   function_f<Functor, int(int, int)>           cb2(functor);
//   function_m<Handler, void(int)>               cb3(h, &Handler::on_tick);
//   function_ct<&free_fn>                        cb4;                    // signature deduced
//   function_ct_m<&Handler::on_tick>             cb5(h);                 // signature deduced
//   function_ct_im<g_handler, &Handler::on_tick> cb6;                    // signature deduced
//
// The callback_registry enforces R = void separately (see callback_registry.h),
// so non-void signatures remain legal for stand-alone callbacks that are not
// dispatched through a registry.
// -----------------------------------------------------------------------------
template <typename Signature>
class i_function;

template <typename R, typename... Args>
class i_function<R(Args...)>
{
public:
    using return_type = R;
    using param_types = std::tuple<Args...>;
    using signature   = R(Args...);

    virtual ~i_function() = default;
    virtual R operator()(Args... args) = 0;
};

// -----------------------------------------------------------------------------
// Runtime-bound free / static function
// Free / static function pointer is stored as a member => one level of
// indirection per call.
//
//   int add(int a, int b);
//   castle::callbacks::function<int(int, int)> cb(&add);
//   int r = cb(1, 2);
// -----------------------------------------------------------------------------
template <typename Signature>
class function;

template <typename R, typename... Args>
class function<R(Args...)> : public i_function<R(Args...)>
{
public:
    using return_type = R;
    using param_types = std::tuple<Args...>;
    using signature   = R(Args...);

    function(R (*func)(Args...)) : func_(func) {}

    R operator()(Args... args) override
    {
        if constexpr (std::is_void_v<R>)
        {
            (*func_)(std::forward<Args>(args)...);
        }
        else
        {
            return (*func_)(std::forward<Args>(args)...);
        }
    }

private:
    R (*func_)(Args...);
};

// -----------------------------------------------------------------------------
// Runtime-bound functor / lambda (owns the callable by value).
// The callable (functor object or lambda closure) is stored as a member,
// so this variant works with lambdas that capture state, std::bind results,
// or any object that provides operator()(Args...) with return type R.
//
//   int x = 10;
//   auto lam = [x](int a, float b) -> void { /* use x */ };
//   castle::callbacks::function_f<decltype(lam), void(int, float)> cb(std::move(lam));
//   cb(1, 2.0f);
//
// Because the closure type of a lambda is anonymous, prefer the make_
// factory below to deduce it:
//
//   auto cb = castle::callbacks::make_function_f<void(int, float)>(
//       [x](int a, float b) { /* ... */ });
// -----------------------------------------------------------------------------
template <typename Callable, typename Signature>
class function_f;

template <typename Callable, typename R, typename... Args>
class function_f<Callable, R(Args...)> : public i_function<R(Args...)>
{
public:
    using callable_type = Callable;
    using return_type   = R;
    using param_types   = std::tuple<Args...>;
    using signature     = R(Args...);

    // Perfect-forward construction so both lvalue functors and rvalue
    // lambda closures can be stored efficiently.
    template <typename C, typename = std::enable_if_t<!std::is_same_v<std::decay_t<C>, function_f>>>
    explicit function_f(C&& c)
        : callable_(std::forward<C>(c)) {}

    R operator()(Args... args) override
    {
        if constexpr (std::is_void_v<R>)
        {
            callable_(std::forward<Args>(args)...);
        }
        else
        {
            return callable_(std::forward<Args>(args)...);
        }
    }

private:
    Callable callable_;
};

// Deduction helper: caller only needs to spell out the signature R(Args...),
// the closure/functor type is deduced.
template <typename Signature, typename Callable>
auto make_function_f(Callable&& c)
{
    return function_f<std::decay_t<Callable>, Signature>(std::forward<Callable>(c));
}

// -----------------------------------------------------------------------------
// Runtime-bound functor by reference (does NOT own the callable).
// Use when the functor is large, non-copyable, or you explicitly want shared
// state. Caller must ensure the referenced functor outlives this callback.
//
//   struct BigFunctor { void operator()(int); /* heavy state */ };
//   BigFunctor f;
//   castle::callbacks::function_fr<BigFunctor, void(int)> cb(f);
//   cb(7);
// -----------------------------------------------------------------------------
template <typename Callable, typename Signature>
class function_fr;

template <typename Callable, typename R, typename... Args>
class function_fr<Callable, R(Args...)> : public i_function<R(Args...)>
{
public:
    using callable_type = Callable;
    using return_type   = R;
    using param_types   = std::tuple<Args...>;
    using signature     = R(Args...);

    explicit function_fr(Callable& c) : callable_(&c) {}

    R operator()(Args... args) override
    {
        if constexpr (std::is_void_v<R>)
        {
            (*callable_)(std::forward<Args>(args)...);
        }
        else
        {
            return (*callable_)(std::forward<Args>(args)...);
        }
    }

private:
    Callable* callable_;
};

template <typename Signature, typename Callable>
auto make_function_fr(Callable& c)
{
    return function_fr<Callable, Signature>(c);
}

// -----------------------------------------------------------------------------
// Runtime-bound member function.
// Object pointer and member function pointer are stored as members
// => one level of indirection per call.
// Caller must ensure the object outlives this callback.
//
//   struct Handler { void on_tick(int, float); };
//   Handler h;
//   castle::callbacks::function_m<Handler, void(int, float)> cb(h, &Handler::on_tick);
//   cb(1, 2.0f);
// -----------------------------------------------------------------------------
template <typename ObjType, typename Signature>
class function_m;

template <typename ObjType, typename R, typename... Args>
class function_m<ObjType, R(Args...)> : public i_function<R(Args...)>
{
public:
    using obj_type    = ObjType;
    using return_type = R;
    using param_types = std::tuple<Args...>;
    using signature   = R(Args...);

    function_m(obj_type& obj, R (obj_type::*func)(Args...))
        : obj_(&obj), func_(func) {}

    R operator()(Args... args) override
    {
        if constexpr (std::is_void_v<R>)
        {
            (obj_->*func_)(std::forward<Args>(args)...);
        }
        else
        {
            return (obj_->*func_)(std::forward<Args>(args)...);
        }
    }

private:
    obj_type* obj_;
    R (obj_type::*func_)(Args...);
};

// -----------------------------------------------------------------------------
// Compile-time-bound free / static function
// Empty object: the call is a direct function call. Signature is deduced
// from the function pointer's type.
//
//   void on_event(int, float);
//   castle::callbacks::function_ct<&on_event> cb;
//   cb(1, 2.0f);
// -----------------------------------------------------------------------------
template <auto Func>
class function_ct;

template <typename R, typename... Args, R (*Func)(Args...)>
class function_ct<Func> : public i_function<R(Args...)>
{
public:
    using return_type = R;
    using param_types = std::tuple<Args...>;
    using signature   = R(Args...);

    R operator()(Args... args) override
    {
        if constexpr (std::is_void_v<R>)
        {
            (*Func)(std::forward<Args>(args)...);
        }
        else
        {
            return (*Func)(std::forward<Args>(args)...);
        }
    }
};

// -----------------------------------------------------------------------------
// Compile-time-bound functor (zero storage).
// Useful for stateless lambdas wrapped in a named type, or functors that are
// default-constructible and pure. The callable is instantiated on each call;
// for stateless closures the compiler collapses this to a direct call.
//
//   struct Add { int operator()(int a, int b) { return a + b; } };
//   castle::callbacks::function_ct_f<Add, int(int, int)> cb;
//   int r = cb(1, 2);
// -----------------------------------------------------------------------------
template <typename Callable, typename Signature>
class function_ct_f;

template <typename Callable, typename R, typename... Args>
class function_ct_f<Callable, R(Args...)> : public i_function<R(Args...)>
{
    static_assert(std::is_default_constructible_v<Callable>,
                  "function_ct_f requires a default-constructible callable "
                  "(stateless functor or captureless lambda wrapped in a type).");
public:
    using callable_type = Callable;
    using return_type   = R;
    using param_types   = std::tuple<Args...>;
    using signature     = R(Args...);

    R operator()(Args... args) override
    {
        if constexpr (std::is_void_v<R>)
        {
            Callable{}(std::forward<Args>(args)...);
        }
        else
        {
            return Callable{}(std::forward<Args>(args)...);
        }
    }
};

// -----------------------------------------------------------------------------
// Compile-time-bound member function, runtime-bound instance.
// The member function pointer is a non-type template parameter so the call is
// devirtualisable/inlinable. Only the object pointer is stored. Signature is
// deduced from the member function pointer's type.
//
//   struct Handler { void on_tick(int, float); };
//   Handler h;
//   castle::callbacks::function_ct_m<&Handler::on_tick> cb(h);
//   cb(1, 2.0f);
// -----------------------------------------------------------------------------
template <auto mem_func_>
class function_ct_m;

template <typename ObjType, typename R, typename... Args, R (ObjType::*mem_func_)(Args...)>
class function_ct_m<mem_func_> : public i_function<R(Args...)>
{
public:
    using obj_type    = ObjType;
    using return_type = R;
    using param_types = std::tuple<Args...>;
    using signature   = R(Args...);

    explicit function_ct_m(obj_type& obj) : obj_(&obj) {}

    R operator()(Args... args) override
    {
        if constexpr (std::is_void_v<R>)
        {
            (obj_->*mem_func_)(std::forward<Args>(args)...);
        }
        else
        {
            return (obj_->*mem_func_)(std::forward<Args>(args)...);
        }
    }

private:
    obj_type* obj_;
};

template <typename ObjType, typename R, typename... Args, R (ObjType::*mem_func_)(Args...) const>
class function_ct_m<mem_func_> : public i_function<R(Args...)>
{
public:
    using obj_type    = ObjType;
    using return_type = R;
    using param_types = std::tuple<Args...>;
    using signature   = R(Args...);

    explicit function_ct_m(const obj_type& obj) : obj_(&obj) {}

    R operator()(Args... args) override
    {
        if constexpr (std::is_void_v<R>)
        {
            (obj_->*mem_func_)(std::forward<Args>(args)...);
        }
        else
        {
            return (obj_->*mem_func_)(std::forward<Args>(args)...);
        }
    }

private:
    const obj_type* obj_;
};

// -----------------------------------------------------------------------------
// Compile-time-bound instance AND member function.
// Nothing is stored per-callback: the call site is a direct member call.
// Instance must have static storage duration (namespace scope, static, etc.).
//
//   struct Handler { void on_tick(int); };
//   Handler g_handler;
//   castle::callbacks::function_ct_im<g_handler, &Handler::on_tick> cb;
//   cb(42);
// -----------------------------------------------------------------------------
template <auto& Instance, auto mem_func_>
class function_ct_im;

template <typename ObjType, ObjType& Instance,
          typename R, typename... Args, R (ObjType::*mem_func_)(Args...)>
class function_ct_im<Instance, mem_func_> : public i_function<R(Args...)>
{
public:
    using obj_type    = ObjType;
    using return_type = R;
    using param_types = std::tuple<Args...>;
    using signature   = R(Args...);

    R operator()(Args... args) override
    {
        if constexpr (std::is_void_v<R>)
        {
            (Instance.*mem_func_)(std::forward<Args>(args)...);
        }
        else
        {
            return (Instance.*mem_func_)(std::forward<Args>(args)...);
        }
    }
};

template <typename ObjType, ObjType& Instance,
          typename R, typename... Args, R (ObjType::*mem_func_)(Args...) const>
class function_ct_im<Instance, mem_func_> : public i_function<R(Args...)>
{
public:
    using obj_type    = ObjType;
    using return_type = R;
    using param_types = std::tuple<Args...>;
    using signature   = R(Args...);

    R operator()(Args... args) override
    {
        if constexpr (std::is_void_v<R>)
        {
            (Instance.*mem_func_)(std::forward<Args>(args)...);
        }
        else
        {
            return (Instance.*mem_func_)(std::forward<Args>(args)...);
        }
    }
};

} // namespace callbacks
} // namespace castle