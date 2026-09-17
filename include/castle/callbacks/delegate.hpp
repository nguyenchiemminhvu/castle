#ifndef CASTLE_CALLBACKS_DELEGATE_HPP
#define CASTLE_CALLBACKS_DELEGATE_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/traits.hpp"
#include "castle/utility/forward.hpp"
#include "castle/utility/move.hpp"
#include "castle/utility/tuple.hpp"

namespace castle
{
namespace callbacks
{

// -----------------------------------------------------------------------------
// Signature-aware base interface. All callback variants below derive from
// this so the caller can hold a single delegate_base<R(Args...)>* regardless of
// how the target is bound. The primary template is left undefined so that a
// mistyped instantiation such as delegate_base<int, float> (i.e. non-signature
// form) fails to compile with a clear "incomplete type" diagnostic.
//
//   template <typename Signature>          class delegate_base;             // undefined
//   template <typename R, typename... Args>
//   class delegate_base<R(Args...)>;                                        // defined
//
// The convention throughout function.h is that every concrete callback type
// is parameterised by a function TYPE (R(Args...)) - the same style used by
// std::function - so R is discovered from the signature via partial
// specialisation. This keeps user-facing spellings uniform:
//
//   function<void(int)>                           cb1(&free_fn);
//   delegate_ft<Functor, int(int, int)>           cb2(functor);
//   delegate_member<Handler, void(int)>           cb3(h, &Handler::on_tick);
//   delegate_ptr_ct<&free_fn>                     cb4;                    // signature deduced
//   delegate_member_ct<&Handler::on_tick>         cb5(h);                 // signature deduced
//   delegate_ins_ct<g_handler, &Handler::on_tick> cb6;                    // signature deduced
//
// The delegate_registry enforces R = void separately (see delegate_registry.h),
// so non-void signatures remain legal for stand-alone callbacks that are not
// dispatched through a registry.
// -----------------------------------------------------------------------------
template <typename Signature>
class delegate_base;

template <typename R, typename... Args>
class delegate_base<R(Args...)>
{
public:
    using return_type = R;
    using param_types = castle::tuple<Args...>;
    using signature   = R(Args...);

    virtual ~delegate_base() CASTLE_DEFAULT;
    virtual R operator()(Args... args) CASTLE_NOEXCEPT = 0;
};

// -----------------------------------------------------------------------------
// Runtime-bound free / static function
// Free / static function pointer is stored as a member => one level of
// indirection per call.
//
//   int add(int a, int b);
//   castle::callbacks::delegate_ptr<int(int, int)> cb(&add);
//   int r = cb(1, 2);
// -----------------------------------------------------------------------------
template <typename Signature>
class delegate_ptr;

template <typename R, typename... Args>
class delegate_ptr<R(Args...)> : public delegate_base<R(Args...)>
{
public:
    using return_type = R;
    using param_types = castle::tuple<Args...>;
    using signature   = R(Args...);

    delegate_ptr(R (*func)(Args...)) : func_(func) {}

    R operator()(Args... args) CASTLE_NOEXCEPT override
    {
        CASTLE_IF_CONSTEXPR (meta::is_void<R>::value)
        {
            (*func_)(CASTLE_FORWARD<Args>(args)...);
        }
        else
        {
            return (*func_)(CASTLE_FORWARD<Args>(args)...);
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
//   castle::callbacks::delegate_ft<decltype(lam), void(int, float)> cb(std::move(lam));
//   cb(1, 2.0f);
//
// Because the closure type of a lambda is anonymous, prefer the make_
// factory below to deduce it:
//
//   auto cb = castle::callbacks::make_delegate_ft<void(int, float)>(
//       [x](int a, float b) { /* ... */ });
// -----------------------------------------------------------------------------
template <typename Callable,
          typename Signature,
          size_type StorageSize = castle::inplace_storage_reserved,
          size_type StorageAlignment = castle::inplace_alignment_default>
class delegate_ft;

template <typename Callable, typename R, typename... Args,
          size_type StorageSize, size_type StorageAlignment>
class delegate_ft<Callable, R(Args...), StorageSize, StorageAlignment> : public delegate_base<R(Args...)>
{
public:
    using callable_type = Callable;
    using return_type   = R;
    using param_types   = castle::tuple<Args...>;
    using signature     = R(Args...);

    // Perfect-forward construction so both lvalue functors and rvalue
    // lambda closures can be stored efficiently.
    template <typename C,
              typename = meta::enable_if_t<!meta::is_same<meta::decay_t<C>, delegate_ft>::value>>
    explicit delegate_ft(C&& c)
        : callable_(CASTLE_FORWARD<C>(c))
    {
        using decayed_callable = meta::decay_t<Callable>;

        static_assert(sizeof(decayed_callable) <= StorageSize,
                      "Callable is too large for function storage");
        static_assert(StorageAlignment != 0U && (StorageAlignment & (StorageAlignment - 1U)) == 0U,
                      "StorageAlignment must be a non-zero power of two");
    }

    R operator()(Args... args) CASTLE_NOEXCEPT override
    {
        CASTLE_IF_CONSTEXPR (meta::is_void<R>::value)
        {
            callable_(CASTLE_FORWARD<Args>(args)...);
        }
        else
        {
            return callable_(CASTLE_FORWARD<Args>(args)...);
        }
    }

private:
    Callable callable_;
};

// Deduction helper: caller only needs to spell out the signature R(Args...),
// the closure/functor type is deduced.
template <typename Signature, typename Callable>
auto make_delegate_ft(Callable&& c)
{
    return delegate_ft<meta::decay_t<Callable>, Signature>(CASTLE_FORWARD<Callable>(c));
}

// -----------------------------------------------------------------------------
// Runtime-bound functor by reference (does NOT own the callable).
// Use when the functor is large, non-copyable, or you explicitly want shared
// state. Caller must ensure the referenced functor outlives this callback.
//
//   struct BigFunctor { void operator()(int); /* heavy state */ };
//   BigFunctor f;
//   castle::callbacks::delegate_ftr<BigFunctor, void(int)> cb(f);
//   cb(7);
// -----------------------------------------------------------------------------
template <typename Callable, typename Signature>
class delegate_ftr;

template <typename Callable, typename R, typename... Args>
class delegate_ftr<Callable, R(Args...)> : public delegate_base<R(Args...)>
{
public:
    using callable_type = Callable;
    using return_type   = R;
    using param_types   = castle::tuple<Args...>;
    using signature     = R(Args...);

    explicit delegate_ftr(Callable& c) : callable_(&c) {}

    R operator()(Args... args) CASTLE_NOEXCEPT override
    {
        CASTLE_IF_CONSTEXPR (meta::is_void<R>::value)
        {
            (*callable_)(CASTLE_FORWARD<Args>(args)...);
        }
        else
        {
            return (*callable_)(CASTLE_FORWARD<Args>(args)...);
        }
    }

private:
    Callable* callable_;
};

template <typename Signature, typename Callable>
auto make_delegate_ftr(Callable& c)
{
    return delegate_ftr<Callable, Signature>(c);
}

// -----------------------------------------------------------------------------
// Runtime-bound member function.
// Object pointer and member function pointer are stored as members
// => one level of indirection per call.
// Caller must ensure the object outlives this callback.
//
//   struct Handler { void on_tick(int, float); };
//   Handler h;
//   castle::callbacks::delegate_member<Handler, void(int, float)> cb(h, &Handler::on_tick);
//   cb(1, 2.0f);
// -----------------------------------------------------------------------------
template <typename ObjType, typename Signature>
class delegate_member;

template <typename ObjType, typename R, typename... Args>
class delegate_member<ObjType, R(Args...)> : public delegate_base<R(Args...)>
{
public:
    using obj_type    = ObjType;
    using return_type = R;
    using param_types = castle::tuple<Args...>;
    using signature   = R(Args...);

    delegate_member(obj_type& obj, R (obj_type::*func)(Args...))
        : obj_(&obj), func_(func) {}

    R operator()(Args... args) CASTLE_NOEXCEPT override
    {
        CASTLE_IF_CONSTEXPR (meta::is_void<R>::value)
        {
            (obj_->*func_)(CASTLE_FORWARD<Args>(args)...);
        }
        else
        {
            return (obj_->*func_)(CASTLE_FORWARD<Args>(args)...); // LCOV_EXCL_BR_LINE
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
//   castle::callbacks::delegate_ptr_ct<&on_event> cb;
//   cb(1, 2.0f);
// -----------------------------------------------------------------------------
template <auto Func>
class delegate_ptr_ct;

template <typename R, typename... Args, R (*Func)(Args...)>
class delegate_ptr_ct<Func> : public delegate_base<R(Args...)>
{
public:
    using return_type = R;
    using param_types = castle::tuple<Args...>;
    using signature   = R(Args...);

    R operator()(Args... args) CASTLE_NOEXCEPT override
    {
        CASTLE_IF_CONSTEXPR (meta::is_void<R>::value)
        {
            (*Func)(CASTLE_FORWARD<Args>(args)...);
        }
        else
        {
            return (*Func)(CASTLE_FORWARD<Args>(args)...);
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
//   castle::callbacks::delegate_ft_ct<Add, int(int, int)> cb;
//   int r = cb(1, 2);
// -----------------------------------------------------------------------------
template <typename Callable, typename Signature>
class delegate_ft_ct;

template <typename Callable, typename R, typename... Args>
class delegate_ft_ct<Callable, R(Args...)> : public delegate_base<R(Args...)>
{
    static_assert(meta::is_default_constructible<Callable>::value,
                  "delegate_ft_ct requires a default-constructible callable "
                  "(stateless functor or captureless lambda wrapped in a type).");
public:
    using callable_type = Callable;
    using return_type   = R;
    using param_types   = castle::tuple<Args...>;
    using signature     = R(Args...);

    R operator()(Args... args) CASTLE_NOEXCEPT override
    {
        CASTLE_IF_CONSTEXPR (meta::is_void<R>::value)
        {
            Callable{}(CASTLE_FORWARD<Args>(args)...);
        }
        else
        {
            return Callable{}(CASTLE_FORWARD<Args>(args)...);
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
//   castle::callbacks::delegate_member_ct<&Handler::on_tick> cb(h);
//   cb(1, 2.0f);
// -----------------------------------------------------------------------------
template <auto mem_func_>
class delegate_member_ct;

template <typename ObjType, typename R, typename... Args, R (ObjType::*mem_func_)(Args...)>
class delegate_member_ct<mem_func_> : public delegate_base<R(Args...)>
{
public:
    using obj_type    = ObjType;
    using return_type = R;
    using param_types = castle::tuple<Args...>;
    using signature   = R(Args...);

    explicit delegate_member_ct(obj_type& obj) : obj_(&obj) {}

    R operator()(Args... args) CASTLE_NOEXCEPT override
    {
        CASTLE_IF_CONSTEXPR (meta::is_void<R>::value)
        {
            (obj_->*mem_func_)(CASTLE_FORWARD<Args>(args)...);
        }
        else
        {
            return (obj_->*mem_func_)(CASTLE_FORWARD<Args>(args)...);
        }
    }

private:
    obj_type* obj_;
};

template <typename ObjType, typename R, typename... Args, R (ObjType::*mem_func_)(Args...) CASTLE_CONST>
class delegate_member_ct<mem_func_> : public delegate_base<R(Args...)>
{
public:
    using obj_type    = ObjType;
    using return_type = R;
    using param_types = castle::tuple<Args...>;
    using signature   = R(Args...);

    explicit delegate_member_ct(CASTLE_CONST obj_type& obj) : obj_(&obj) {}

    R operator()(Args... args) CASTLE_NOEXCEPT override
    {
        CASTLE_IF_CONSTEXPR (meta::is_void<R>::value)
        {
            (obj_->*mem_func_)(CASTLE_FORWARD<Args>(args)...);
        }
        else
        {
            return (obj_->*mem_func_)(CASTLE_FORWARD<Args>(args)...);
        }
    }

private:
    CASTLE_CONST obj_type* obj_;
};

// -----------------------------------------------------------------------------
// Compile-time-bound instance AND member function.
// Nothing is stored per-callback: the call site is a direct member call.
// Instance must have static storage duration (namespace scope, static, etc.).
//
//   struct Handler { void on_tick(int); };
//   Handler g_handler;
//   castle::callbacks::delegate_ins_ct<g_handler, &Handler::on_tick> cb;
//   cb(42);
// -----------------------------------------------------------------------------
template <auto& Instance, auto mem_func_>
class delegate_ins_ct;

template <typename ObjType, ObjType& Instance,
          typename R, typename... Args, R (ObjType::*mem_func_)(Args...)>
class delegate_ins_ct<Instance, mem_func_> : public delegate_base<R(Args...)>
{
public:
    using obj_type    = ObjType;
    using return_type = R;
    using param_types = castle::tuple<Args...>;
    using signature   = R(Args...);

    R operator()(Args... args) CASTLE_NOEXCEPT override
    {
        CASTLE_IF_CONSTEXPR (meta::is_void<R>::value)
        {
            (Instance.*mem_func_)(CASTLE_FORWARD<Args>(args)...);
        }
        else
        {
            return (Instance.*mem_func_)(CASTLE_FORWARD<Args>(args)...);
        }
    }
};

template <typename ObjType, ObjType& Instance,
          typename R, typename... Args, R (ObjType::*mem_func_)(Args...) CASTLE_CONST>
class delegate_ins_ct<Instance, mem_func_> : public delegate_base<R(Args...)>
{
public:
    using obj_type    = ObjType;
    using return_type = R;
    using param_types = castle::tuple<Args...>;
    using signature   = R(Args...);

    R operator()(Args... args) CASTLE_NOEXCEPT override
    {
        CASTLE_IF_CONSTEXPR (meta::is_void<R>::value)
        {
            (Instance.*mem_func_)(CASTLE_FORWARD<Args>(args)...);
        }
        else
        {
            return (Instance.*mem_func_)(CASTLE_FORWARD<Args>(args)...);
        }
    }
};

} // namespace callbacks
} // namespace castle

#endif // CASTLE_CALLBACKS_DELEGATE_HPP
