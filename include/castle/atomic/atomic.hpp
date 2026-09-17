// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @brief Embedded-friendly atomic storage and fence primitives.
 *
 * Use this header when shared state must be accessed atomically without STL atomics,
 * heap allocation, exceptions, or RTTI. Integral and pointer specializations forward
 * directly to compiler `__atomic` builtins, while trivially copyable non-integral types
 * fall back to deterministic mutex-serialized access.
 *
 * @code
 * #include "castle/atomic/atomic.hpp"
 *
 * castle::atomic<unsigned> counter{0U};
 * counter.fetch_add(1U, castle::memory_order_relaxed);
 * unsigned snapshot = counter.load(castle::memory_order_acquire);
 * (void)snapshot;
 * @endcode
 */
#ifndef CASTLE_ATOMIC_ATOMIC_HPP
#define CASTLE_ATOMIC_ATOMIC_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/sync/mutex.hpp"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace castle
{

/**
 * @brief Memory-order constants forwarded to compiler atomic builtins.
 */
enum memory_order
{
    memory_order_relaxed = __ATOMIC_RELAXED, /**< No ordering beyond atomicity. */
    memory_order_consume = __ATOMIC_CONSUME, /**< Consume ordering when supported by the compiler. */
    memory_order_acquire = __ATOMIC_ACQUIRE, /**< Acquire ordering for reads and successful read-modify-write operations. */
    memory_order_release = __ATOMIC_RELEASE, /**< Release ordering for writes and successful read-modify-write operations. */
    memory_order_acq_rel = __ATOMIC_ACQ_REL, /**< Combined acquire and release ordering. */
    memory_order_seq_cst = __ATOMIC_SEQ_CST  /**< Sequentially consistent ordering. */
};

namespace detail
{

/**
 * @brief Traits for atomic types, indicating whether they are always lock-free.
 *
 * @tparam IsAlwaysLockFree Indicates whether the atomic type is always lock-free.
 * @note Specializations for integral types typically set this to `true`.
 */
template <bool IsAlwaysLockFree>
struct atomic_traits
{
    static CASTLE_CONSTEXPR bool is_always_lock_free = IsAlwaysLockFree;
};

template <bool IsAlwaysLockFree>
CASTLE_CONSTEXPR bool atomic_traits<IsAlwaysLockFree>::is_always_lock_free;

}

/**
 * @brief Atomic wrapper for integral types.
 *
 * @tparam T Stored integral type.
 * @tparam IntegralType Internal specialization selector.
 *
 * @note This specialization uses compiler `__atomic` builtins directly.
 */
template <typename T, bool IntegralType = meta::is_integral<T>::value>
class atomic : public detail::atomic_traits<IntegralType>
{
public:
    /** @brief Alias for the stored value type. */
    using value_type = T;

    /** @brief Constructs the atomic with a value-initialized payload. */
    CASTLE_CONSTEXPR atomic() CASTLE_NOEXCEPT : value_(T()) {}
    /**
     * @brief Constructs the atomic with an initial value.
     *
     * @param v Initial stored value.
     */
    CASTLE_CONSTEXPR atomic(T v) CASTLE_NOEXCEPT : value_(v) {}

    atomic(CASTLE_CONST atomic&) CASTLE_DELETE;
    atomic& operator=(CASTLE_CONST atomic&) CASTLE_DELETE;
    atomic& operator=(CASTLE_CONST atomic&) CASTLE_VOLATILE CASTLE_DELETE;

    /**
     * @brief Stores a value through assignment syntax.
     *
     * @param v Value to store.
     * @return The stored value.
     */
    T operator=(T v) CASTLE_NOEXCEPT
    {
        store(v);
        return v;
    }
    T operator=(T v) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        store(v);
        return v;
    }

    /**
     * @brief Atomically increments the value and returns the result.
     *
     * @return Incremented value.
     */
    T operator++() CASTLE_NOEXCEPT
    {
        return __atomic_add_fetch(&value_, 1, __ATOMIC_SEQ_CST);
    }
    T operator++() CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_add_fetch(&value_, 1, __ATOMIC_SEQ_CST);
    }

    /**
     * @brief Atomically increments the value and returns the previous value.
     *
     * @return Value before increment.
     */
    T operator++(int) CASTLE_NOEXCEPT
    {
        return __atomic_fetch_add(&value_, 1, __ATOMIC_SEQ_CST);
    }
    T operator++(int) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_fetch_add(&value_, 1, __ATOMIC_SEQ_CST);
    }

    /**
     * @brief Atomically decrements the value and returns the result.
     *
     * @return Decremented value.
     */
    T operator--() CASTLE_NOEXCEPT
    {
        return __atomic_sub_fetch(&value_, 1, __ATOMIC_SEQ_CST);
    }
    T operator--() CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_sub_fetch(&value_, 1, __ATOMIC_SEQ_CST);
    }

    /**
     * @brief Atomically decrements the value and returns the previous value.
     *
     * @return Value before decrement.
     */
    T operator--(int) CASTLE_NOEXCEPT
    {
        return __atomic_fetch_sub(&value_, 1, __ATOMIC_SEQ_CST);
    }
    T operator--(int) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_fetch_sub(&value_, 1, __ATOMIC_SEQ_CST);
    }

    /**
     * @brief Atomically adds a value and returns the result.
     *
     * @param v Amount to add.
     * @return Updated stored value.
     */
    T operator+=(T v) CASTLE_NOEXCEPT
    {
        return __atomic_add_fetch(&value_, v, __ATOMIC_SEQ_CST);
    }
    T operator+=(T v) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_add_fetch(&value_, v, __ATOMIC_SEQ_CST);
    }

    /**
     * @brief Atomically subtracts a value and returns the result.
     *
     * @param v Amount to subtract.
     * @return Updated stored value.
     */
    T operator-=(T v) CASTLE_NOEXCEPT
    {
        return __atomic_sub_fetch(&value_, v, __ATOMIC_SEQ_CST);
    }
    T operator-=(T v) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_sub_fetch(&value_, v, __ATOMIC_SEQ_CST);
    }

    /**
     * @brief Atomically applies bitwise AND and returns the result.
     *
     * @param v Mask value.
     * @return Updated stored value.
     */
    T operator&=(T v) CASTLE_NOEXCEPT
    {
        return __atomic_and_fetch(&value_, v, __ATOMIC_SEQ_CST);
    }
    T operator&=(T v) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_and_fetch(&value_, v, __ATOMIC_SEQ_CST);
    }

    /**
     * @brief Atomically applies bitwise OR and returns the result.
     *
     * @param v Mask value.
     * @return Updated stored value.
     */
    T operator|=(T v) CASTLE_NOEXCEPT
    {
        return __atomic_or_fetch(&value_, v, __ATOMIC_SEQ_CST);
    }
    T operator|=(T v) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_or_fetch(&value_, v, __ATOMIC_SEQ_CST);
    }

    /**
     * @brief Atomically applies bitwise XOR and returns the result.
     *
     * @param v Mask value.
     * @return Updated stored value.
     */
    T operator^=(T v) CASTLE_NOEXCEPT
    {
        return __atomic_xor_fetch(&value_, v, __ATOMIC_SEQ_CST);
    }
    T operator^=(T v) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_xor_fetch(&value_, v, __ATOMIC_SEQ_CST);
    }

    /**
     * @brief Loads the stored value through implicit conversion.
     *
     * @return Current stored value.
     */
    operator T() CASTLE_CONST CASTLE_NOEXCEPT { return load(); }
    operator T() CASTLE_CONST CASTLE_VOLATILE CASTLE_NOEXCEPT { return load(); }

    /**
     * @brief Reports whether the target can access this atomic lock-free at runtime.
     *
     * @return `true` when the runtime reports lock-free access; otherwise `false`.
     */
    bool is_lock_free() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return __atomic_is_lock_free(sizeof(T), &value_);
    }
    bool is_lock_free() CASTLE_CONST CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_is_lock_free(sizeof(T), &value_);
    }

    /**
     * @brief Stores a value with the selected memory order.
     *
     * @param v Value to store.
     * @param order Memory ordering applied to the store.
     */
    void store(T v, memory_order order = memory_order_seq_cst) CASTLE_NOEXCEPT
    {
        __atomic_store_n(&value_, v, order);
    }
    void store(T v, memory_order order = memory_order_seq_cst) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        __atomic_store_n(&value_, v, order);
    }

    /**
     * @brief Loads the current value with the selected memory order.
     *
     * @param order Memory ordering applied to the load.
     * @return Current stored value.
     */
    T load(memory_order order = memory_order_seq_cst) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return __atomic_load_n(&value_, order);
    }
    T load(memory_order order = memory_order_seq_cst) CASTLE_CONST CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_load_n(&value_, order);
    }

    /**
     * @brief Replaces the stored value and returns the previous one.
     *
     * @param v New value.
     * @param order Memory ordering applied to the exchange.
     * @return Previous stored value.
     */
    T exchange(T v, memory_order order = memory_order_seq_cst) CASTLE_NOEXCEPT
    {
        return __atomic_exchange_n(&value_, v, order);
    }
    T exchange(T v, memory_order order = memory_order_seq_cst) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_exchange_n(&value_, v, order);
    }

    /**
     * @brief Adds a value and returns the previous stored value.
     *
     * @param v Amount to add.
     * @param order Memory ordering applied to the operation.
     * @return Value before the addition.
     */
    T fetch_add(T v, memory_order order = memory_order_seq_cst) CASTLE_NOEXCEPT
    {
        return __atomic_fetch_add(&value_, v, order);
    }
    T fetch_add(T v, memory_order order = memory_order_seq_cst) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_fetch_add(&value_, v, order);
    }

    /**
     * @brief Subtracts a value and returns the previous stored value.
     *
     * @param v Amount to subtract.
     * @param order Memory ordering applied to the operation.
     * @return Value before the subtraction.
     */
    T fetch_sub(T v, memory_order order = memory_order_seq_cst) CASTLE_NOEXCEPT
    {
        return __atomic_fetch_sub(&value_, v, order);
    }
    T fetch_sub(T v, memory_order order = memory_order_seq_cst) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_fetch_sub(&value_, v, order);
    }

    /**
     * @brief Applies bitwise AND and returns the previous stored value.
     *
     * @param v Mask value.
     * @param order Memory ordering applied to the operation.
     * @return Value before the AND operation.
     */
    T fetch_and(T v, memory_order order = memory_order_seq_cst) CASTLE_NOEXCEPT
    {
        return __atomic_fetch_and(&value_, v, order);
    }
    T fetch_and(T v, memory_order order = memory_order_seq_cst) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_fetch_and(&value_, v, order);
    }

    /**
     * @brief Applies bitwise OR and returns the previous stored value.
     *
     * @param v Mask value.
     * @param order Memory ordering applied to the operation.
     * @return Value before the OR operation.
     */
    T fetch_or(T v, memory_order order = memory_order_seq_cst) CASTLE_NOEXCEPT
    {
        return __atomic_fetch_or(&value_, v, order);
    }
    T fetch_or(T v, memory_order order = memory_order_seq_cst) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_fetch_or(&value_, v, order);
    }

    /**
     * @brief Applies bitwise XOR and returns the previous stored value.
     *
     * @param v Mask value.
     * @param order Memory ordering applied to the operation.
     * @return Value before the XOR operation.
     */
    T fetch_xor(T v, memory_order order = memory_order_seq_cst) CASTLE_NOEXCEPT
    {
        return __atomic_fetch_xor(&value_, v, order);
    }
    T fetch_xor(T v, memory_order order = memory_order_seq_cst) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_fetch_xor(&value_, v, order);
    }

    /**
     * @brief Performs a weak compare-and-exchange operation.
     *
     * @param expected Expected current value; updated with the actual value on failure.
     * @param desired Replacement value written on success.
     * @param success Memory order used on success.
     * @param failure Memory order used on failure.
     * @return `true` when the exchange succeeded; otherwise `false`.
     */
    bool compare_exchange_weak(T& expected, T desired,
                               memory_order success, memory_order failure) CASTLE_NOEXCEPT
    {
        return __atomic_compare_exchange_n(&value_, &expected, desired, true, success, failure);
    }
    bool compare_exchange_weak(T& expected, T desired,
                               memory_order success, memory_order failure) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_compare_exchange_n(&value_, &expected, desired, true, success, failure);
    }
    bool compare_exchange_weak(T& expected, T desired,
                               memory_order order = memory_order_seq_cst) CASTLE_NOEXCEPT
    {
        return compare_exchange_weak(expected, desired, order, order);
    }
    bool compare_exchange_weak(T& expected, T desired,
                               memory_order order = memory_order_seq_cst) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return compare_exchange_weak(expected, desired, order, order);
    }

    /**
     * @brief Performs a strong compare-and-exchange operation.
     *
     * @param expected Expected current value; updated with the actual value on failure.
     * @param desired Replacement value written on success.
     * @param success Memory order used on success.
     * @param failure Memory order used on failure.
     * @return `true` when the exchange succeeded; otherwise `false`.
     */
    bool compare_exchange_strong(T& expected, T desired,
                                 memory_order success, memory_order failure) CASTLE_NOEXCEPT
    {
        return __atomic_compare_exchange_n(&value_, &expected, desired, false, success, failure);
    }
    bool compare_exchange_strong(T& expected, T desired,
                                 memory_order success, memory_order failure) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_compare_exchange_n(&value_, &expected, desired, false, success, failure);
    }
    bool compare_exchange_strong(T& expected, T desired,
                                 memory_order order = memory_order_seq_cst) CASTLE_NOEXCEPT
    {
        return compare_exchange_strong(expected, desired, order, order);
    }
    bool compare_exchange_strong(T& expected, T desired,
                                 memory_order order = memory_order_seq_cst) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return compare_exchange_strong(expected, desired, order, order);
    }

private:
    CASTLE_MUTABLE T value_;
};

/**
 * @brief Atomic wrapper specialization for pointers.
 *
 * @tparam T Pointee type.
 *
 * @note Pointer arithmetic follows `std::atomic<T*>` semantics and scales by `sizeof(T)`.
 */
template <typename T>
class atomic<T*, false> : public detail::atomic_traits<true>
{
public:
    /** @brief Alias for the stored pointer type. */
    using value_type      = T*;
    /** @brief Alias for pointer-difference arithmetic. */
    using difference_type = ptrdiff_t;

    /** @brief Constructs the atomic with a null pointer. */
    CASTLE_CONSTEXPR atomic() CASTLE_NOEXCEPT : value_(nullptr) {}
    /**
     * @brief Constructs the atomic with an initial pointer.
     *
     * @param v Initial stored pointer.
     */
    CASTLE_CONSTEXPR atomic(T* v) CASTLE_NOEXCEPT : value_(v) {}

    atomic(CASTLE_CONST atomic&) CASTLE_DELETE;
    atomic& operator=(CASTLE_CONST atomic&) CASTLE_DELETE;
    atomic& operator=(CASTLE_CONST atomic&) CASTLE_VOLATILE CASTLE_DELETE;

    /**
     * @brief Stores a pointer through assignment syntax.
     *
     * @param v Pointer to store.
     * @return The stored pointer.
     */
    T* operator=(T* v) CASTLE_NOEXCEPT
    {
        store(v);
        return v;
    }
    T* operator=(T* v) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        store(v);
        return v;
    }

    /**
     * @brief Advances the pointer by one element and returns the result.
     *
     * @return Incremented pointer.
     */
    T* operator++() CASTLE_NOEXCEPT
    {
        return fetch_add(1) + 1;
    }
    T* operator++() CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return fetch_add(1) + 1;
    }
    T* operator++(int) CASTLE_NOEXCEPT
    {
        return fetch_add(1);
    }
    T* operator++(int) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return fetch_add(1);
    }

    /**
     * @brief Moves the pointer back by one element and returns the result.
     *
     * @return Decremented pointer.
     */
    T* operator--() CASTLE_NOEXCEPT
    {
        return fetch_sub(1) - 1;
    }
    T* operator--() CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return fetch_sub(1) - 1;
    }
    T* operator--(int) CASTLE_NOEXCEPT
    {
        return fetch_sub(1);
    }
    T* operator--(int) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return fetch_sub(1);
    }

    /**
     * @brief Advances the pointer by `v` elements and returns the result.
     *
     * @param v Element count to add.
     * @return Updated pointer.
     */
    T* operator+=(ptrdiff_t v) CASTLE_NOEXCEPT
    {
        return fetch_add(v) + v;
    }
    T* operator+=(ptrdiff_t v) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return fetch_add(v) + v;
    }

    /**
     * @brief Moves the pointer back by `v` elements and returns the result.
     *
     * @param v Element count to subtract.
     * @return Updated pointer.
     */
    T* operator-=(ptrdiff_t v) CASTLE_NOEXCEPT
    {
        return fetch_sub(v) - v;
    }
    T* operator-=(ptrdiff_t v) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return fetch_sub(v) - v;
    }

    /**
     * @brief Loads the stored pointer through implicit conversion.
     *
     * @return Current stored pointer.
     */
    operator T*() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return load();
    }
    operator T*() CASTLE_CONST CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return load();
    }

    /**
     * @brief Reports whether the target can access this pointer atomic lock-free at runtime.
     *
     * @return `true` when the runtime reports lock-free access; otherwise `false`.
     */
    bool is_lock_free() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return __atomic_is_lock_free(sizeof(T*), &value_);
    }
    bool is_lock_free() CASTLE_CONST CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_is_lock_free(sizeof(T*), &value_);
    }

    /**
     * @brief Stores a pointer with the selected memory order.
     *
     * @param v Pointer to store.
     * @param order Memory ordering applied to the store.
     */
    void store(T* v, memory_order order = memory_order_seq_cst) CASTLE_NOEXCEPT
    {
        __atomic_store_n(&value_, v, order);
    }
    void store(T* v, memory_order order = memory_order_seq_cst) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        __atomic_store_n(&value_, v, order);
    }

    /**
     * @brief Loads the current pointer with the selected memory order.
     *
     * @param order Memory ordering applied to the load.
     * @return Current stored pointer.
     */
    T* load(memory_order order = memory_order_seq_cst) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return __atomic_load_n(&value_, order);
    }
    T* load(memory_order order = memory_order_seq_cst) CASTLE_CONST CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_load_n(&value_, order);
    }

    /**
     * @brief Replaces the stored pointer and returns the previous one.
     *
     * @param v New pointer value.
     * @param order Memory ordering applied to the exchange.
     * @return Previous stored pointer.
     */
    T* exchange(T* v, memory_order order = memory_order_seq_cst) CASTLE_NOEXCEPT
    {
        return __atomic_exchange_n(&value_, v, order);
    }
    T* exchange(T* v, memory_order order = memory_order_seq_cst) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_exchange_n(&value_, v, order);
    }

    /**
     * @brief Advances the pointer by `v` elements and returns the previous pointer.
     *
     * @param v Element count to add.
     * @param order Memory ordering applied to the operation.
     * @return Pointer value before the addition.
     */
    T* fetch_add(ptrdiff_t v, memory_order order = memory_order_seq_cst) CASTLE_NOEXCEPT
    {
        return __atomic_fetch_add(&value_, v * static_cast<ptrdiff_t>(sizeof(T)), order);
    }
    T* fetch_add(ptrdiff_t v, memory_order order = memory_order_seq_cst) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_fetch_add(&value_, v * static_cast<ptrdiff_t>(sizeof(T)), order);
    }

    /**
     * @brief Moves the pointer back by `v` elements and returns the previous pointer.
     *
     * @param v Element count to subtract.
     * @param order Memory ordering applied to the operation.
     * @return Pointer value before the subtraction.
     */
    T* fetch_sub(ptrdiff_t v, memory_order order = memory_order_seq_cst) CASTLE_NOEXCEPT
    {
        return __atomic_fetch_sub(&value_, v * static_cast<ptrdiff_t>(sizeof(T)), order);
    }
    T* fetch_sub(ptrdiff_t v, memory_order order = memory_order_seq_cst) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_fetch_sub(&value_, v * static_cast<ptrdiff_t>(sizeof(T)), order);
    }

    /**
     * @brief Performs a weak compare-and-exchange operation on the pointer.
     *
     * @param expected Expected pointer; updated with the actual value on failure.
     * @param desired Replacement pointer written on success.
     * @param success Memory order used on success.
     * @param failure Memory order used on failure.
     * @return `true` when the exchange succeeded; otherwise `false`.
     */
    bool compare_exchange_weak(T*& expected, T* desired,
                               memory_order success, memory_order failure) CASTLE_NOEXCEPT
    {
        return __atomic_compare_exchange_n(&value_, &expected, desired, true, success, failure);
    }
    bool compare_exchange_weak(T*& expected, T* desired,
                               memory_order success, memory_order failure) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_compare_exchange_n(&value_, &expected, desired, true, success, failure);
    }
    bool compare_exchange_weak(T*& expected, T* desired,
                               memory_order order = memory_order_seq_cst) CASTLE_NOEXCEPT
    {
        return compare_exchange_weak(expected, desired, order, order);
    }
    bool compare_exchange_weak(T*& expected, T* desired,
                               memory_order order = memory_order_seq_cst) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return compare_exchange_weak(expected, desired, order, order);
    }

    /**
     * @brief Performs a strong compare-and-exchange operation on the pointer.
     *
     * @param expected Expected pointer; updated with the actual value on failure.
     * @param desired Replacement pointer written on success.
     * @param success Memory order used on success.
     * @param failure Memory order used on failure.
     * @return `true` when the exchange succeeded; otherwise `false`.
     */
    bool compare_exchange_strong(T*& expected, T* desired,
                                 memory_order success, memory_order failure) CASTLE_NOEXCEPT
    {
        return __atomic_compare_exchange_n(&value_, &expected, desired, false, success, failure);
    }
    bool compare_exchange_strong(T*& expected, T* desired,
                                 memory_order success, memory_order failure) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_compare_exchange_n(&value_, &expected, desired, false, success, failure);
    }
    bool compare_exchange_strong(T*& expected, T* desired,
                                 memory_order order = memory_order_seq_cst) CASTLE_NOEXCEPT
    {
        return compare_exchange_strong(expected, desired, order, order);
    }
    bool compare_exchange_strong(T*& expected, T* desired,
                                 memory_order order = memory_order_seq_cst) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return compare_exchange_strong(expected, desired, order, order);
    }

private:
    CASTLE_MUTABLE T* value_;
};

/**
 * @brief Mutex-backed atomic wrapper for trivially copyable non-integral types.
 *
 * @tparam T Stored trivially copyable type.
 *
 * @note This specialization is deterministic but not lock-free.
 */
template <typename T>
class atomic<T, false> : public detail::atomic_traits<false>
{
    static_assert(meta::is_trivially_copyable<T>::value,
                  "castle::atomic<T> requires T to be trivially copyable");
    static_assert(meta::is_copy_constructible<T>::value,
                  "castle::atomic<T> requires T to be copy constructible");
    static_assert(meta::is_copy_assignable<T>::value,
                  "castle::atomic<T> requires T to be copy assignable");

public:
    /** @brief Alias for the stored value type. */
    using value_type = T;

    /** @brief Constructs the atomic with a value-initialized payload. */
    atomic() : value_(T()) {}
    /**
     * @brief Constructs the atomic with an initial value.
     *
     * @param v Initial stored value.
     */
    atomic(T v) : value_(v) {}

    atomic(CASTLE_CONST atomic&) CASTLE_DELETE;
    atomic& operator=(CASTLE_CONST atomic&) CASTLE_DELETE;

    /**
     * @brief Stores a value through assignment syntax.
     *
     * @param v Value to store.
     * @return The stored value.
     */
    T operator=(T v)
    {
        store(v);
        return v;
    }

    /**
     * @brief Loads the stored value through implicit conversion.
     *
     * @return Current stored value.
     */
    operator T() CASTLE_CONST
    {
        return load();
    }

    /**
     * @brief Reports whether this specialization is lock-free.
     *
     * @return Always `false` for the mutex-backed fallback.
     */
    bool is_lock_free() CASTLE_CONST
    {
        return false;
    }

    /**
     * @brief Stores a value while holding the fallback mutex.
     *
     * @param v Value to store.
     * @param order Ignored memory-order parameter retained for interface compatibility.
     */
    void store(T v, memory_order  = memory_order_seq_cst)
    {
        lock_.lock();
        value_ = v;
        lock_.unlock();
    }

    /**
     * @brief Loads the current value while holding the fallback mutex.
     *
     * @param order Ignored memory-order parameter retained for interface compatibility.
     * @return Current stored value.
     */
    T load(memory_order  = memory_order_seq_cst) CASTLE_CONST
    {
        lock_.lock();
        T result = value_;
        lock_.unlock();
        return result;
    }

    /**
     * @brief Replaces the stored value while holding the fallback mutex.
     *
     * @param v New value.
     * @param order Ignored memory-order parameter retained for interface compatibility.
     * @return Previous stored value.
     */
    T exchange(T v, memory_order  = memory_order_seq_cst)
    {
        lock_.lock();
        T result = value_;
        value_   = v;
        lock_.unlock();
        return result;
    }

    /**
     * @brief Performs a compare-and-exchange using bitwise comparison under the fallback mutex.
     *
     * @param expected Expected value; updated with the actual value on failure.
     * @param desired Replacement value written on success.
     * @param success Ignored memory-order parameter retained for interface compatibility.
     * @param failure Ignored memory-order parameter retained for interface compatibility.
     * @return `true` when the exchange succeeded; otherwise `false`.
     */
    bool compare_exchange_weak(T& expected, T desired,
                               memory_order , memory_order )
    {
        lock_.lock();
        bool ok = false;
        if (memcmp(&value_, &expected, sizeof(T)) == 0)
        {
            value_ = desired;
            ok = true;
        }
        else
        {
            expected = value_;
            ok = false;
        }
        lock_.unlock();
        return ok;
    }
    bool compare_exchange_weak(T& expected, T desired,
                               memory_order order = memory_order_seq_cst)
    {
        return compare_exchange_weak(expected, desired, order, order);
    }
    bool compare_exchange_strong(T& expected, T desired,
                                 memory_order success, memory_order failure)
    {
        return compare_exchange_weak(expected, desired, success, failure);
    }
    bool compare_exchange_strong(T& expected, T desired,
                                 memory_order order = memory_order_seq_cst)
    {
        return compare_exchange_weak(expected, desired, order, order);
    }

private:
    CASTLE_MUTABLE castle::mutex lock_;
    T                     value_;
};

/**
 * @brief Issues a thread fence with the requested memory order.
 *
 * @param order Fence memory ordering.
 */
CASTLE_INLINE void atomic_thread_fence(memory_order order) CASTLE_NOEXCEPT
{
    __atomic_thread_fence(order);
}

/**
 * @brief Issues a signal fence with the requested memory order.
 *
 * @param order Fence memory ordering.
 */
CASTLE_INLINE void atomic_signal_fence(memory_order order) CASTLE_NOEXCEPT
{
    __atomic_signal_fence(order);
}

/**
 * @brief Type aliases for atomic types corresponding to fundamental integral and pointer types.
 */
using atomic_bool   = atomic<bool>;
using atomic_char   = atomic<char>;
using atomic_schar  = atomic<signed char>;
using atomic_uchar  = atomic<unsigned char>;
using atomic_short  = atomic<short>;
using atomic_ushort = atomic<unsigned short>;
using atomic_int    = atomic<int>;
using atomic_uint   = atomic<unsigned int>;
using atomic_long   = atomic<long>;
using atomic_ulong  = atomic<unsigned long>;
using atomic_llong  = atomic<long long>;
using atomic_ullong = atomic<unsigned long long>;

/**
 * @brief Type aliases for atomic types corresponding to fixed-width integer types.
 */
using atomic_int8_t   = atomic<int8_t>;
using atomic_uint8_t  = atomic<uint8_t>;
using atomic_int16_t  = atomic<int16_t>;
using atomic_uint16_t = atomic<uint16_t>;
using atomic_int32_t  = atomic<int32_t>;
using atomic_uint32_t = atomic<uint32_t>;
using atomic_int64_t  = atomic<int64_t>;
using atomic_uint64_t = atomic<uint64_t>;

/**
 * @brief Type aliases for atomic types corresponding to pointer-sized and other special integral types.
 */
using atomic_intptr_t  = atomic<intptr_t>;
using atomic_uintptr_t = atomic<uintptr_t>;
using atomic_size_t    = atomic<size_t>;
using atomic_ptrdiff_t = atomic<ptrdiff_t>;

}

#endif