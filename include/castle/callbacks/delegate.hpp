// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file delegate.hpp
 * @brief Defines signature-aware delegate wrappers for free functions, functors, and member functions without heap allocation or RTTI.
 *
 * Use this header when you want a lightweight callable object with a fixed binding model known at compile time or
 * construction time. These delegates can store function pointers, callable objects, object references, or compile-time
 * bindings, and they integrate with delegate_registry through the shared delegate_base interface.
 *
 * Key constraints:
 * - No heap allocation.
 * - No RTTI-based type erasure.
 * - Ownership depends on the delegate variant: some store the callable by value, others borrow a function object or instance.
 * - No internal synchronization; delegates are not thread-safe by themselves.
 * - Registries store pointers to delegate_base, so callback objects must outlive any active delegate_registry subscription.
 *
 * @code
 * #include "castle/callbacks/delegate.hpp"
 *
 * struct device
 * {
 *     int total = 0;
 *
 *     void add(int value) noexcept
 *     {
 *         total += value;
 *     }
 * };
 *
 * void add_free(int& total, int value) noexcept
 * {
 *     total += value;
 * }
 *
 * int main()
 * {
 *     device dev{};
 *     castle::callbacks::delegate_member<device, void(int)> member{dev, &device::add};
 *     member(2);
 *
 *     int total = 0;
 *     auto functor = castle::callbacks::make_delegate_ft<void(int)>(
 *         [&total](int value) noexcept
 *         {
 *             add_free(total, value);
 *         });
 *     functor(3);
 *
 *     return (dev.total == 2 && total == 3) ? 0 : 1;
 * }
 * @endcode
 */
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

/**
 * @brief Forward declaration for the common delegate interface.
 * @tparam Signature Function signature in the form `R(Args...)`.
 * @note Only the signature specialization is defined.
 */
template <typename Signature>
class delegate_base;

/**
 * @brief Abstract base interface for delegates with a fixed function signature.
 * @tparam R Return type.
 * @tparam Args Parameter types.
 *
 * Registries borrow pointers to this interface so they can store heterogeneous delegate implementations that all expose
 * the same signature.
 *
 * @warning This base class is non-owning and provides no lifetime management for derived delegate objects.
 */
template <typename R, typename... Args>
class delegate_base<R(Args...)>
{
public:
    using return_type = R;
    using param_types = castle::tuple<Args...>;
    using signature   = R(Args...);

    /**
     * @brief Destroys the delegate interface.
     */
    virtual ~delegate_base() CASTLE_DEFAULT;

    /**
     * @brief Invokes the bound target.
     * @param args Arguments forwarded to the bound target.
     * @return The bound target's return value when `R` is non-void.
     */
    virtual R operator()(Args... args) CASTLE_NOEXCEPT = 0;
};

/**
 * @brief Forward declaration for a runtime-bound free-function delegate.
 * @tparam Signature Function signature in the form `R(Args...)`.
 */
template <typename Signature>
class delegate_ptr;

/**
 * @brief Stores a runtime free-function or static-member-function pointer.
 * @tparam R Return type.
 * @tparam Args Parameter types.
 *
 * This variant owns only the function pointer itself. Invocation forwards directly to the pointed-to function.
 */
template <typename R, typename... Args>
class delegate_ptr<R(Args...)> : public delegate_base<R(Args...)>
{
public:
    using return_type = R;
    using param_types = castle::tuple<Args...>;
    using signature   = R(Args...);

    /**
     * @brief Binds a free function or static member function.
     * @param func Function pointer to invoke.
     * @warning Passing nullptr compiles but calling operator() would be invalid.
     */
    delegate_ptr(R (*func)(Args...)) : func_(func) {}

    /**
     * @brief Invokes the bound function pointer.
     * @param args Arguments forwarded to the function.
     * @return The function result when `R` is non-void.
     */
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

/**
 * @brief Forward declaration for an owning runtime functor delegate.
 * @tparam Callable Stored callable type.
 * @tparam Signature Function signature in the form `R(Args...)`.
 * @tparam StorageSize Maximum supported callable size checked at compile time.
 * @tparam StorageAlignment Required callable alignment checked at compile time.
 */
template <typename Callable,
          typename Signature,
          size_type StorageSize = castle::inplace_storage_reserved,
          size_type StorageAlignment = castle::inplace_alignment_default>
class delegate_ft;

/**
 * @brief Stores a callable object or lambda by value.
 * @tparam Callable Stored callable type.
 * @tparam R Return type.
 * @tparam Args Parameter types.
 * @tparam StorageSize Maximum supported callable size checked at compile time.
 * @tparam StorageAlignment Required callable alignment checked at compile time.
 *
 * This variant owns the callable object directly inside the delegate and performs only compile-time size and alignment
 * checks. It is suitable for small stateful lambdas and functors.
 */
template <typename Callable, typename R, typename... Args,
          size_type StorageSize, size_type StorageAlignment>
class delegate_ft<Callable, R(Args...), StorageSize, StorageAlignment> : public delegate_base<R(Args...)>
{
public:
    using callable_type = Callable;
    using return_type   = R;
    using param_types   = castle::tuple<Args...>;
    using signature     = R(Args...);

    /**
     * @brief Constructs the delegate from a callable object.
     * @tparam C Source callable type.
     * @param c Callable object to store by value.
     * @warning Compilation fails if the decayed callable exceeds StorageSize or violates StorageAlignment.
     */
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

    /**
     * @brief Invokes the stored callable object.
     * @param args Arguments forwarded to the callable.
     * @return The callable result when `R` is non-void.
     */
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

/**
 * @brief Creates an owning functor delegate while deducing the callable type.
 * @tparam Signature Function signature in the form `R(Args...)`.
 * @tparam Callable Source callable type.
 * @param c Callable object to store by value.
 * @return delegate_ft<meta::decay_t<Callable>, Signature>.
 */
template <typename Signature, typename Callable>
auto make_delegate_ft(Callable&& c)
{
    return delegate_ft<meta::decay_t<Callable>, Signature>(CASTLE_FORWARD<Callable>(c));
}

/**
 * @brief Forward declaration for a non-owning functor-reference delegate.
 * @tparam Callable Referenced callable type.
 * @tparam Signature Function signature in the form `R(Args...)`.
 */
template <typename Callable, typename Signature>
class delegate_ftr;

/**
 * @brief Stores a pointer to an existing callable object.
 * @tparam Callable Referenced callable type.
 * @tparam R Return type.
 * @tparam Args Parameter types.
 *
 * This variant does not copy the callable object. It is useful for large or shared functors, but the referenced object
 * must outlive the delegate.
 */
template <typename Callable, typename R, typename... Args>
class delegate_ftr<Callable, R(Args...)> : public delegate_base<R(Args...)>
{
public:
    using callable_type = Callable;
    using return_type   = R;
    using param_types   = castle::tuple<Args...>;
    using signature     = R(Args...);

    /**
     * @brief Binds a callable object by reference.
     * @param c Callable object that must remain alive while the delegate is used.
     */
    explicit delegate_ftr(Callable& c) : callable_(&c) {}

    /**
     * @brief Invokes the referenced callable object.
     * @param args Arguments forwarded to the callable.
     * @return The callable result when `R` is non-void.
     */
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

/**
 * @brief Creates a non-owning functor-reference delegate while preserving the callable type.
 * @tparam Signature Function signature in the form `R(Args...)`.
 * @tparam Callable Referenced callable type.
 * @param c Callable object that must outlive the returned delegate.
 * @return delegate_ftr<Callable, Signature>.
 */
template <typename Signature, typename Callable>
auto make_delegate_ftr(Callable& c)
{
    return delegate_ftr<Callable, Signature>(c);
}

/**
 * @brief Forward declaration for a runtime-bound member-function delegate.
 * @tparam ObjType Object type.
 * @tparam Signature Function signature in the form `R(Args...)`.
 */
template <typename ObjType, typename Signature>
class delegate_member;

/**
 * @brief Stores an object pointer and a member-function pointer selected at runtime.
 * @tparam ObjType Object type.
 * @tparam R Return type.
 * @tparam Args Parameter types.
 *
 * The object is borrowed, not owned. The referenced object must outlive the delegate.
 */
template <typename ObjType, typename R, typename... Args>
class delegate_member<ObjType, R(Args...)> : public delegate_base<R(Args...)>
{
public:
    using obj_type    = ObjType;
    using return_type = R;
    using param_types = castle::tuple<Args...>;
    using signature   = R(Args...);

    /**
     * @brief Binds an object instance and a member function pointer.
     * @param obj Object instance to invoke on.
     * @param func Member function pointer to invoke.
     * @warning The object must outlive the delegate.
     */
    delegate_member(obj_type& obj, R (obj_type::*func)(Args...))
        : obj_(&obj), func_(func) {}

    /**
     * @brief Invokes the bound member function.
     * @param args Arguments forwarded to the member function.
     * @return The member function result when `R` is non-void.
     */
    R operator()(Args... args) CASTLE_NOEXCEPT override
    {
        CASTLE_IF_CONSTEXPR (meta::is_void<R>::value)
        {
            (obj_->*func_)(CASTLE_FORWARD<Args>(args)...); // LCOV_EXCL_BR_LINE
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

/**
 * @brief Forward declaration for a compile-time-bound free-function delegate.
 * @tparam Func Free-function pointer supplied as a non-type template argument.
 */
template <auto Func>
class delegate_ptr_ct;

/**
 * @brief Binds a free function at compile time and stores no runtime state.
 * @tparam R Return type.
 * @tparam Args Parameter types.
 * @tparam Func Free-function pointer.
 */
template <typename R, typename... Args, R (*Func)(Args...)>
class delegate_ptr_ct<Func> : public delegate_base<R(Args...)>
{
public:
    using return_type = R;
    using param_types = castle::tuple<Args...>;
    using signature   = R(Args...);

    /**
     * @brief Invokes the compile-time-bound function.
     * @param args Arguments forwarded to the function.
     * @return The function result when `R` is non-void.
     */
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

/**
 * @brief Forward declaration for a compile-time functor delegate.
 * @tparam Callable Default-constructible callable type.
 * @tparam Signature Function signature in the form `R(Args...)`.
 */
template <typename Callable, typename Signature>
class delegate_ft_ct;

/**
 * @brief Invokes a default-constructible callable type without storing runtime state.
 * @tparam Callable Default-constructible callable type.
 * @tparam R Return type.
 * @tparam Args Parameter types.
 *
 * A fresh temporary Callable object is created for each invocation.
 */
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

    /**
     * @brief Invokes a temporary default-constructed Callable.
     * @param args Arguments forwarded to the callable.
     * @return The callable result when `R` is non-void.
     */
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

/**
 * @brief Forward declaration for a compile-time-bound member-function delegate with a runtime instance.
 * @tparam mem_func_ Member-function pointer supplied as a non-type template argument.
 */
template <auto mem_func_>
class delegate_member_ct;

/**
 * @brief Stores an object pointer and binds a non-const member function at compile time.
 * @tparam ObjType Object type.
 * @tparam R Return type.
 * @tparam Args Parameter types.
 * @tparam mem_func_ Member-function pointer.
 */
template <typename ObjType, typename R, typename... Args, R (ObjType::*mem_func_)(Args...)>
class delegate_member_ct<mem_func_> : public delegate_base<R(Args...)>
{
public:
    using obj_type    = ObjType;
    using return_type = R;
    using param_types = castle::tuple<Args...>;
    using signature   = R(Args...);

    /**
     * @brief Binds an object instance for a compile-time-selected member function.
     * @param obj Object instance to invoke on.
     * @warning The object must outlive the delegate.
     */
    explicit delegate_member_ct(obj_type& obj) : obj_(&obj) {}

    /**
     * @brief Invokes the compile-time-bound member function.
     * @param args Arguments forwarded to the member function.
     * @return The member function result when `R` is non-void.
     */
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

/**
 * @brief Stores a pointer to a const object and binds a const member function at compile time.
 * @tparam ObjType Object type.
 * @tparam R Return type.
 * @tparam Args Parameter types.
 * @tparam mem_func_ Const member-function pointer.
 */
template <typename ObjType, typename R, typename... Args, R (ObjType::*mem_func_)(Args...) CASTLE_CONST>
class delegate_member_ct<mem_func_> : public delegate_base<R(Args...)>
{
public:
    using obj_type    = ObjType;
    using return_type = R;
    using param_types = castle::tuple<Args...>;
    using signature   = R(Args...);

    /**
     * @brief Binds a const object instance for a compile-time-selected const member function.
     * @param obj Const object instance to invoke on.
     * @warning The object must outlive the delegate.
     */
    explicit delegate_member_ct(CASTLE_CONST obj_type& obj) : obj_(&obj) {}

    /**
     * @brief Invokes the compile-time-bound const member function.
     * @param args Arguments forwarded to the member function.
     * @return The member function result when `R` is non-void.
     */
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

/**
 * @brief Forward declaration for a compile-time-bound instance-and-member delegate.
 * @tparam Instance Object instance supplied as a reference non-type template argument.
 * @tparam mem_func_ Member-function pointer supplied as a non-type template argument.
 */
template <auto& Instance, auto mem_func_>
class delegate_ins_ct;

/**
 * @brief Binds both the instance and a non-const member function at compile time.
 * @tparam ObjType Object type.
 * @tparam Instance Referenced object instance.
 * @tparam R Return type.
 * @tparam Args Parameter types.
 * @tparam mem_func_ Member-function pointer.
 *
 * This variant stores no runtime state. The referenced instance must have static storage duration or otherwise outlive
 * every use of the delegate object.
 */
template <typename ObjType, ObjType& Instance,
          typename R, typename... Args, R (ObjType::*mem_func_)(Args...)>
class delegate_ins_ct<Instance, mem_func_> : public delegate_base<R(Args...)>
{
public:
    using obj_type    = ObjType;
    using return_type = R;
    using param_types = castle::tuple<Args...>;
    using signature   = R(Args...);

    /**
     * @brief Invokes the compile-time-bound member function on the compile-time-bound instance.
     * @param args Arguments forwarded to the member function.
     * @return The member function result when `R` is non-void.
     */
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

/**
 * @brief Binds both the instance and a const member function at compile time.
 * @tparam ObjType Object type.
 * @tparam Instance Referenced object instance.
 * @tparam R Return type.
 * @tparam Args Parameter types.
 * @tparam mem_func_ Const member-function pointer.
 */
template <typename ObjType, ObjType& Instance,
          typename R, typename... Args, R (ObjType::*mem_func_)(Args...) CASTLE_CONST>
class delegate_ins_ct<Instance, mem_func_> : public delegate_base<R(Args...)>
{
public:
    using obj_type    = ObjType;
    using return_type = R;
    using param_types = castle::tuple<Args...>;
    using signature   = R(Args...);

    /**
     * @brief Invokes the compile-time-bound const member function on the compile-time-bound instance.
     * @param args Arguments forwarded to the member function.
     * @return The member function result when `R` is non-void.
     */
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
