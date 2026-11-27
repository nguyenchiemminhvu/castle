// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file soo_buffer.hpp
 * @brief Stores one object inside a fixed inline buffer with no heap allocation.
 *
 * Use this header when an object must live inside caller-owned storage whose capacity is known
 * at compile time. The buffer is aligned for both `T` and `T*`, never allocates dynamically,
 * and exposes raw lifetime management through construction and destruction of the contained
 * object. Callers must ensure the inline bytes always contain a live `T` before `get()` or the
 * destructor are used, and must observe the move-member caveats documented below.
 *
 * @code
 * castle::memory::soo_buffer<MyType> buffer(MyType(7U));
 * MyType* pointer = buffer.get();
 * @endcode
 */
#ifndef CASTLE_MEMORY_SOO_BUFFER_HPP
#define CASTLE_MEMORY_SOO_BUFFER_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/utility/utility.hpp"
#include "castle/memory/lifetime.hpp"
#include "castle/memory/new.hpp"

#include <stdint.h>

namespace castle
{
namespace memory
{

/**
 * @brief Inline buffer that stores one object without dynamic allocation.
 * @tparam T Object type accessed through the buffer.
 * @tparam StackSize Number of inline bytes reserved for the object.
 * @note Construction is enabled only when `sizeof(T) <= StackSize`.
 * @warning This class does not track whether the inline bytes currently contain a live `T`.
 * The constructor and `get()`/destructor assume the buffer holds `T`, and the move members
 * explicitly invoke destructors. Use only with carefully managed lifetimes.
 */
template <typename T, size_type StackSize = sizeof(T)>
class soo_buffer
{
public:
    /**
     * @brief Constructs an object directly inside the inline buffer.
     * @tparam Arg Type deduced from the forwarded argument.
     * @param arg Value forwarded into placement construction.
     * @note This overload participates only when `sizeof(T) <= StackSize`.
     * @warning The implementation constructs `Arg` and later treats the storage as `T`.
     * Pass an rvalue whose deduced `Arg` is `T` so the buffer actually contains a live `T`.
     */
    template <typename Arg, typename = meta::enable_if_t<sizeof(T) <= StackSize, void>>
    soo_buffer(Arg&& arg) CASTLE_NOEXCEPT
    {
        ::new (static_cast<void*>(inline_buffer)) Arg(CASTLE_FORWARD<Arg>(arg));
    }

    /**
     * @brief Copy construction is disabled.
     * @param Unused source buffer.
     * @note `soo_buffer` does not provide copy semantics.
     * @warning Use explicit reconstruction instead of copying.
     */
    soo_buffer(CASTLE_CONST soo_buffer&) CASTLE_DELETE;

    /**
     * @brief Copy assignment is disabled.
     * @param Unused source buffer.
     * @return This function is deleted.
     * @note `soo_buffer` does not provide copy semantics.
     * @warning Use explicit reconstruction instead of copying.
     */
    soo_buffer& operator=(CASTLE_CONST soo_buffer&) CASTLE_DELETE;

    /**
     * @brief Move-constructs this buffer from another buffer.
     * @param other Source buffer.
     * @note The implementation destroys `*this`, move-constructs a `T` into the inline bytes,
     * and then destroys `other`.
     * @warning This member directly invokes both buffer destructors. It is only safe when those
     * destructor calls are valid under external lifetime management and will not be repeated later.
     */
    soo_buffer(soo_buffer&& other) CASTLE_NOEXCEPT
    {
        this->~soo_buffer();
        ::new (static_cast<void*>(inline_buffer)) T(CASTLE_MOVE(*other.get()));
        other.~soo_buffer();
    }

    /**
     * @brief Move-assigns from another buffer.
     * @param other Source buffer.
     * @return Reference to `*this`.
     * @note The implementation destroys `*this`, move-constructs a `T` into the inline bytes,
     * and then destroys `other` when the buffers are distinct.
     * @warning This member directly invokes both buffer destructors. It is only safe when those
     * destructor calls are valid under external lifetime management and will not be repeated later.
     */
    soo_buffer& operator=(soo_buffer&& other) CASTLE_NOEXCEPT
    {
        if (this != &other)
        {
            this->~soo_buffer();
            ::new (static_cast<void*>(inline_buffer)) T(CASTLE_MOVE(*other.get()));
            other.~soo_buffer();
        }
        return *this;
    }

    /**
     * @brief Destroys the object currently stored in the inline buffer.
     * @return Nothing.
     * @note The stored object is accessed through `launder(reinterpret_cast<T*>(inline_buffer))`.
     * @warning A live `T` object must currently occupy the buffer, and its lifetime must end exactly once.
     */
    ~soo_buffer()
    {
        castle::memory::launder(reinterpret_cast<T*>(inline_buffer))->~T();
    }

    /**
     * @brief Returns a mutable pointer to the stored object.
     * @return Laundered pointer to the inline `T`.
     * @note The returned pointer refers to storage inside this object; no allocation occurs.
     * @warning A live `T` object must currently occupy the inline buffer.
     */
    T* get() CASTLE_NOEXCEPT
    {
        return castle::memory::launder(reinterpret_cast<T*>(inline_buffer));
    }

    /**
     * @brief Returns a const pointer to the stored object.
     * @return Laundered const pointer to the inline `T`.
     * @note The returned pointer refers to storage inside this object; no allocation occurs.
     * @warning A live `T` object must currently occupy the inline buffer.
     */
    CASTLE_CONST T* get() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return castle::memory::launder(reinterpret_cast<CASTLE_CONST T*>(inline_buffer));
    }

private:
    union alignas(alignof(T) > alignof(T*) ? alignof(T) : alignof(T*)) // Keep the inline bytes aligned for both T and T*.
    {
        char inline_buffer[StackSize];
    };
};

}
}

#endif // CASTLE_MEMORY_SOO_BUFFER_HPP
