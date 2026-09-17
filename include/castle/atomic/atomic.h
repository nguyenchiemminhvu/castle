#ifndef CASTLE_ATOMIC_ATOMIC_H
#define CASTLE_ATOMIC_ATOMIC_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"
#include "castle/mutex/mutex.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace castle
{

// Memory ordering values are mapped directly onto the compiler's __ATOMIC_*
// constants so that they can be forwarded to the __atomic builtins without
// translation. The enumerators mirror std::memory_order to keep call sites
// source-compatible with the standard library.
enum memory_order
{
    memory_order_relaxed = __ATOMIC_RELAXED,
    memory_order_consume = __ATOMIC_CONSUME,
    memory_order_acquire = __ATOMIC_ACQUIRE,
    memory_order_release = __ATOMIC_RELEASE,
    memory_order_acq_rel = __ATOMIC_ACQ_REL,
    memory_order_seq_cst = __ATOMIC_SEQ_CST
};

namespace detail
{

// Exposes the is_always_lock_free trait required by the standard interface.
// Lock-free specialisations inherit with true, the mutex-backed fallback with
// false.
template <bool IsAlwaysLockFree>
struct atomic_traits
{
    static CASTLE_CONSTEXPR bool is_always_lock_free = IsAlwaysLockFree;
};

template <bool IsAlwaysLockFree>
CASTLE_CONSTEXPR bool atomic_traits<IsAlwaysLockFree>::is_always_lock_free;

} // namespace detail

//===========================================================================
// Primary template: integral T. Lock-free via __atomic builtins.
//===========================================================================
template <typename T, bool IntegralType = castle::is_integral<T>::value>
class atomic : public detail::atomic_traits<IntegralType>
{
public:
    using value_type = T;

    CASTLE_CONSTEXPR atomic() CASTLE_NOEXCEPT : value_(T()) {}
    CASTLE_CONSTEXPR atomic(T v) CASTLE_NOEXCEPT : value_(v) {}

    atomic(CASTLE_CONST atomic&) CASTLE_DELETE;
    atomic& operator=(CASTLE_CONST atomic&) CASTLE_DELETE;
    atomic& operator=(CASTLE_CONST atomic&) CASTLE_VOLATILE CASTLE_DELETE;

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

    T operator++() CASTLE_NOEXCEPT
    {
        return __atomic_add_fetch(&value_, 1, __ATOMIC_SEQ_CST);
    }
    T operator++() CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_add_fetch(&value_, 1, __ATOMIC_SEQ_CST);
    }

    T operator++(int) CASTLE_NOEXCEPT
    {
        return __atomic_fetch_add(&value_, 1, __ATOMIC_SEQ_CST);
    }
    T operator++(int) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_fetch_add(&value_, 1, __ATOMIC_SEQ_CST);
    }
    
    T operator--() CASTLE_NOEXCEPT
    {
        return __atomic_sub_fetch(&value_, 1, __ATOMIC_SEQ_CST);
    }
    T operator--() CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_sub_fetch(&value_, 1, __ATOMIC_SEQ_CST);
    }
    
    T operator--(int) CASTLE_NOEXCEPT
    {
        return __atomic_fetch_sub(&value_, 1, __ATOMIC_SEQ_CST);
    }
    T operator--(int) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_fetch_sub(&value_, 1, __ATOMIC_SEQ_CST);
    }

    T operator+=(T v) CASTLE_NOEXCEPT
    {
        return __atomic_add_fetch(&value_, v, __ATOMIC_SEQ_CST);
    }
    T operator+=(T v) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_add_fetch(&value_, v, __ATOMIC_SEQ_CST);
    }
    
    T operator-=(T v) CASTLE_NOEXCEPT
    {
        return __atomic_sub_fetch(&value_, v, __ATOMIC_SEQ_CST);
    }
    T operator-=(T v) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_sub_fetch(&value_, v, __ATOMIC_SEQ_CST);
    }
    
    T operator&=(T v) CASTLE_NOEXCEPT
    {
        return __atomic_and_fetch(&value_, v, __ATOMIC_SEQ_CST);
    }
    T operator&=(T v) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_and_fetch(&value_, v, __ATOMIC_SEQ_CST);
    }
    
    T operator|=(T v) CASTLE_NOEXCEPT
    {
        return __atomic_or_fetch(&value_, v, __ATOMIC_SEQ_CST);
    }
    T operator|=(T v) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_or_fetch(&value_, v, __ATOMIC_SEQ_CST);
    }
    
    T operator^=(T v) CASTLE_NOEXCEPT
    {
        return __atomic_xor_fetch(&value_, v, __ATOMIC_SEQ_CST);
    }
    T operator^=(T v) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_xor_fetch(&value_, v, __ATOMIC_SEQ_CST);
    }

    operator T() CASTLE_CONST CASTLE_NOEXCEPT { return load(); }
    operator T() CASTLE_CONST CASTLE_VOLATILE CASTLE_NOEXCEPT { return load(); }

    bool is_lock_free() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return __atomic_is_lock_free(sizeof(T), &value_);
    }
    bool is_lock_free() CASTLE_CONST CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_is_lock_free(sizeof(T), &value_);
    }

    void store(T v, memory_order order = memory_order_seq_cst) CASTLE_NOEXCEPT
    {
        __atomic_store_n(&value_, v, order);
    }
    void store(T v, memory_order order = memory_order_seq_cst) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        __atomic_store_n(&value_, v, order);
    }

    T load(memory_order order = memory_order_seq_cst) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return __atomic_load_n(&value_, order);
    }
    T load(memory_order order = memory_order_seq_cst) CASTLE_CONST CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_load_n(&value_, order);
    }

    T exchange(T v, memory_order order = memory_order_seq_cst) CASTLE_NOEXCEPT
    {
        return __atomic_exchange_n(&value_, v, order);
    }
    T exchange(T v, memory_order order = memory_order_seq_cst) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_exchange_n(&value_, v, order);
    }

    T fetch_add(T v, memory_order order = memory_order_seq_cst) CASTLE_NOEXCEPT
    {
        return __atomic_fetch_add(&value_, v, order);
    }
    T fetch_add(T v, memory_order order = memory_order_seq_cst) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_fetch_add(&value_, v, order);
    }

    T fetch_sub(T v, memory_order order = memory_order_seq_cst) CASTLE_NOEXCEPT
    {
        return __atomic_fetch_sub(&value_, v, order);
    }
    T fetch_sub(T v, memory_order order = memory_order_seq_cst) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_fetch_sub(&value_, v, order);
    }

    T fetch_and(T v, memory_order order = memory_order_seq_cst) CASTLE_NOEXCEPT
    {
        return __atomic_fetch_and(&value_, v, order);
    }
    T fetch_and(T v, memory_order order = memory_order_seq_cst) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_fetch_and(&value_, v, order);
    }

    T fetch_or(T v, memory_order order = memory_order_seq_cst) CASTLE_NOEXCEPT
    {
        return __atomic_fetch_or(&value_, v, order);
    }
    T fetch_or(T v, memory_order order = memory_order_seq_cst) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_fetch_or(&value_, v, order);
    }

    T fetch_xor(T v, memory_order order = memory_order_seq_cst) CASTLE_NOEXCEPT
    {
        return __atomic_fetch_xor(&value_, v, order);
    }
    T fetch_xor(T v, memory_order order = memory_order_seq_cst) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_fetch_xor(&value_, v, order);
    }

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

//===========================================================================
// Pointer specialisation. Arithmetic scales by sizeof(T) as std::atomic<T*>.
//===========================================================================
template <typename T>
class atomic<T*, false> : public detail::atomic_traits<true>
{
public:
    using value_type      = T*;
    using difference_type = ptrdiff_t;

    CASTLE_CONSTEXPR atomic() CASTLE_NOEXCEPT : value_(nullptr) {}
    CASTLE_CONSTEXPR atomic(T* v) CASTLE_NOEXCEPT : value_(v) {}

    atomic(CASTLE_CONST atomic&) CASTLE_DELETE;
    atomic& operator=(CASTLE_CONST atomic&) CASTLE_DELETE;
    atomic& operator=(CASTLE_CONST atomic&) CASTLE_VOLATILE CASTLE_DELETE;

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

    T* operator+=(ptrdiff_t v) CASTLE_NOEXCEPT
    {
        return fetch_add(v) + v;
    }
    T* operator+=(ptrdiff_t v) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return fetch_add(v) + v;
    }

    T* operator-=(ptrdiff_t v) CASTLE_NOEXCEPT
    {
        return fetch_sub(v) - v;
    }
    T* operator-=(ptrdiff_t v) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return fetch_sub(v) - v;
    }

    operator T*() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return load();
    }
    operator T*() CASTLE_CONST CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return load();
    }

    bool is_lock_free() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return __atomic_is_lock_free(sizeof(T*), &value_);
    }
    bool is_lock_free() CASTLE_CONST CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_is_lock_free(sizeof(T*), &value_);
    }

    void store(T* v, memory_order order = memory_order_seq_cst) CASTLE_NOEXCEPT
    {
        __atomic_store_n(&value_, v, order);
    }
    void store(T* v, memory_order order = memory_order_seq_cst) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        __atomic_store_n(&value_, v, order);
    }

    T* load(memory_order order = memory_order_seq_cst) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return __atomic_load_n(&value_, order);
    }
    T* load(memory_order order = memory_order_seq_cst) CASTLE_CONST CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_load_n(&value_, order);
    }

    T* exchange(T* v, memory_order order = memory_order_seq_cst) CASTLE_NOEXCEPT
    {
        return __atomic_exchange_n(&value_, v, order);
    }
    T* exchange(T* v, memory_order order = memory_order_seq_cst) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_exchange_n(&value_, v, order);
    }

    // Scale explicitly by sizeof(T): __atomic_fetch_add treats the pointer as
    // a plain memory location and expects a byte offset.
    T* fetch_add(ptrdiff_t v, memory_order order = memory_order_seq_cst) CASTLE_NOEXCEPT
    {
        return __atomic_fetch_add(&value_, v * static_cast<ptrdiff_t>(sizeof(T)), order);
    }
    T* fetch_add(ptrdiff_t v, memory_order order = memory_order_seq_cst) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_fetch_add(&value_, v * static_cast<ptrdiff_t>(sizeof(T)), order);
    }

    T* fetch_sub(ptrdiff_t v, memory_order order = memory_order_seq_cst) CASTLE_NOEXCEPT
    {
        return __atomic_fetch_sub(&value_, v * static_cast<ptrdiff_t>(sizeof(T)), order);
    }
    T* fetch_sub(ptrdiff_t v, memory_order order = memory_order_seq_cst) CASTLE_VOLATILE CASTLE_NOEXCEPT
    {
        return __atomic_fetch_sub(&value_, v * static_cast<ptrdiff_t>(sizeof(T)), order);
    }

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

//===========================================================================
// Fallback for trivially copyable non-integral, non-pointer types. Access is
// serialised through castle::mutex, so the object is not lock-free.
//===========================================================================
template <typename T>
class atomic<T, false> : public detail::atomic_traits<false>
{
    static_assert(castle::is_trivially_copyable<T>::value,
                  "castle::atomic<T> requires T to be trivially copyable");
    static_assert(castle::is_copy_constructible<T>::value,
                  "castle::atomic<T> requires T to be copy constructible");
    static_assert(castle::is_copy_assignable<T>::value,
                  "castle::atomic<T> requires T to be copy assignable");

public:
    using value_type = T;

    atomic() : value_(T()) {}
    atomic(T v) : value_(v) {}

    atomic(CASTLE_CONST atomic&) CASTLE_DELETE;
    atomic& operator=(CASTLE_CONST atomic&) CASTLE_DELETE;

    T operator=(T v)
    {
        store(v);
        return v;
    }

    operator T() CASTLE_CONST
    {
        return load();
    }

    bool is_lock_free() CASTLE_CONST
    {
        return false;
    }

    void store(T v, memory_order /*order*/ = memory_order_seq_cst)
    {
        lock_.lock();
        value_ = v;
        lock_.unlock();
    }

    T load(memory_order /*order*/ = memory_order_seq_cst) CASTLE_CONST
    {
        lock_.lock();
        T result = value_;
        lock_.unlock();
        return result;
    }

    T exchange(T v, memory_order /*order*/ = memory_order_seq_cst)
    {
        lock_.lock();
        T result = value_;
        value_   = v;
        lock_.unlock();
        return result;
    }

    // Bitwise compare mirrors std::atomic's contract, which is well-defined
    // precisely because T is required to be trivially copyable.
    bool compare_exchange_weak(T& expected, T desired,
                               memory_order /*success*/, memory_order /*failure*/)
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

//===========================================================================
// Free-function fences, mirroring std::atomic_thread_fence / _signal_fence.
//===========================================================================
CASTLE_INLINE void atomic_thread_fence(memory_order order) CASTLE_NOEXCEPT
{
    __atomic_thread_fence(order);
}

CASTLE_INLINE void atomic_signal_fence(memory_order order) CASTLE_NOEXCEPT
{
    __atomic_signal_fence(order);
}

//===========================================================================
// Convenience typedefs matching the standard library.
//===========================================================================
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

using atomic_int8_t   = atomic<int8_t>;
using atomic_uint8_t  = atomic<uint8_t>;
using atomic_int16_t  = atomic<int16_t>;
using atomic_uint16_t = atomic<uint16_t>;
using atomic_int32_t  = atomic<int32_t>;
using atomic_uint32_t = atomic<uint32_t>;
using atomic_int64_t  = atomic<int64_t>;
using atomic_uint64_t = atomic<uint64_t>;

using atomic_intptr_t  = atomic<intptr_t>;
using atomic_uintptr_t = atomic<uintptr_t>;
using atomic_size_t    = atomic<size_t>;
using atomic_ptrdiff_t = atomic<ptrdiff_t>;

} // namespace castle

#endif // CASTLE_ATOMIC_ATOMIC_H