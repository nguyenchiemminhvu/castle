// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file function.hpp
 * @brief Defines an owning, small-buffer callable wrapper that performs type erasure without heap allocation or RTTI.
 *
 * Use this header when a component needs to store an arbitrary callable behind a stable signature, but captured state
 * must stay inline and deterministic. The wrapper keeps the target object in an internal byte buffer and dispatches
 * through stored function pointers rather than RTTI or dynamic allocation.
 *
 * Key constraints:
 * - No heap allocation.
 * - Fixed compile-time storage size and alignment.
 * - Empty wrappers assert on operator() according to the configured CASTLE assertion policy.
 * - Copy and move behavior depends on the stored callable's constructors.
 * - No internal synchronization; sharing one wrapper across threads requires external coordination.
 *
 * @code
 * #include "castle/callbacks/function.hpp"
 *
 * int main()
 * {
 *     int total = 0;
 *     castle::callbacks::function<void(int), 32U> callback{
 *         [&total](int value) noexcept
 *         {
 *             total += value;
 *         }
 *     };
 *
 *     if (callback)
 *     {
 *         callback(4);
 *     }
 *
 *     return total == 4 ? 0 : 1;
 * }
 * @endcode
 */
#ifndef CASTLE_CALLBACKS_FUNCTION_HPP
#define CASTLE_CALLBACKS_FUNCTION_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/utility/move.hpp"
#include "castle/utility/forward.hpp"
#include "castle/memory/new.hpp"

#include <stdint.h>

namespace castle
{
namespace callbacks
{

/**
 * @brief Forward declaration for an owning callable wrapper with inline storage.
 * @tparam Signature Function signature in the form `R(Args...)`.
 * @tparam StorageSize Number of inline bytes reserved for the callable object.
 * @tparam StorageAlignment Alignment of the inline storage buffer.
 */
template <typename Signature,
          size_type StorageSize = castle::inplace_storage_reserved,
          size_type StorageAlignment = castle::inplace_alignment_default>
class function;

/**
 * @brief Stores one callable object in-place and invokes it through a vtable-less function-pointer table.
 * @tparam R Return type of the stored callable.
 * @tparam Args Parameter types accepted by the stored callable.
 * @tparam StorageSize Number of inline bytes reserved for the callable object.
 * @tparam StorageAlignment Alignment of the inline storage buffer.
 *
 * The stored target may be a function pointer, lambda, or other callable object that fits in the configured storage and
 * can be invoked as `R(Args...)`. Copy and move operations delegate to small function pointers captured when the target
 * is constructed, so the wrapper itself stays allocation-free and type-erased without RTTI.
 */
template <typename R,
          typename... Args,
          size_type StorageSize,
          size_type StorageAlignment>
class function<R(Args...), StorageSize, StorageAlignment>
{
public:
    using callback_ptr_t = R(*)(Args...);
    using invoke_ptr_t = R(*)(void*, Args...);
    using destroy_ptr_t = void(*)(void*);
    using copy_ptr_t = void(*)(void*, CASTLE_CONST void*);
    using move_ptr_t = void(*)(void*, void*);

    /**
     * @brief Constructs an empty function wrapper.
     *
     * @note operator bool() returns false until a callable is assigned or constructed into the wrapper.
     */
    function() CASTLE_DEFAULT;

    /**
     * @brief Constructs the wrapper from a callable object stored inside the local buffer.
     * @tparam Callable Source callable type.
     * @param callable Callable object to store.
     * @note The callable type is decay-copied into the buffer.
     * @warning Compilation fails if the callable exceeds StorageSize or StorageAlignment constraints.
     */
    template <typename Callable,
          typename = meta::enable_if_t<
              !meta::is_same<
                  meta::decay_t<Callable>,
                  function
              >::value>>
    function(Callable&& callable)
    {
        using decayed_callable = meta::decay_t<Callable>;

        static_assert(sizeof(decayed_callable) <= StorageSize,
                      "Callable is too large for function storage");
        static_assert(StorageAlignment != 0U && (StorageAlignment & (StorageAlignment - 1U)) == 0U,
                      "StorageAlignment must be a non-zero power of two");

        new (storage_) decayed_callable(CASTLE_FORWARD<Callable>(callable));

        // These stored function pointers are the entire type-erasure layer: no RTTI table or heap allocation is needed.
        invoke_ptr_ = [](void* storage, Args... args) -> R
        {
            CASTLE_IF_CONSTEXPR(meta::is_void<R>::value)
            {
                (*static_cast<decayed_callable*>(storage))(CASTLE_FORWARD<Args>(args)...);
            }
            else
            {
                return (*static_cast<decayed_callable*>(storage))(CASTLE_FORWARD<Args>(args)...);
            }
        };

        destroy_ptr_ = [](void* storage)
        {
            static_cast<decayed_callable*>(storage)->~decayed_callable();
        };

        copy_ptr_ = [](void* target_storage, CASTLE_CONST void* source_storage)
        {
            new (target_storage) decayed_callable(*static_cast<CASTLE_CONST decayed_callable*>(source_storage));
        };

        move_ptr_ = [](void* target_storage, void* source_storage)
        {
            new (target_storage) decayed_callable(CASTLE_MOVE(*static_cast<decayed_callable*>(source_storage)));
            static_cast<decayed_callable*>(source_storage)->~decayed_callable();
        };
    }

    /**
     * @brief Copy-constructs the wrapper from another wrapper.
     * @param other Source wrapper.
     * @note If @p other is empty, the result is empty.
     */
    function(CASTLE_CONST function& other)
    {
        this->invoke_ptr_ = other.invoke_ptr_;
        this->destroy_ptr_ = other.destroy_ptr_;
        this->copy_ptr_ = other.copy_ptr_;
        this->move_ptr_ = other.move_ptr_;
        if (this->copy_ptr_)
        {
            this->copy_ptr_(this->storage_, other.storage_);
        }
    }

    /**
     * @brief Move-constructs the wrapper from another wrapper.
     * @param other Source wrapper.
     * @note If @p other is non-empty, its stored callable is moved and @p other becomes empty.
     */
    function(function&& other) CASTLE_NOEXCEPT
    {
        this->invoke_ptr_ = other.invoke_ptr_;
        this->destroy_ptr_ = other.destroy_ptr_;
        this->copy_ptr_ = other.copy_ptr_;
        this->move_ptr_ = other.move_ptr_;
        if (this->move_ptr_)
        {
            this->move_ptr_(this->storage_, other.storage_);
        }

        other.reset_pointers();
    }

    /**
     * @brief Copy-assigns from another wrapper.
     * @param other Source wrapper.
     * @return This wrapper.
     * @note The current target, if any, is destroyed before copying the new one.
     */
    function& operator=(CASTLE_CONST function& other)
    {
        if (this != &other)
        {
            if (this->destroy_ptr_)
            {
                this->destroy_ptr_(this->storage_);
            }
            this->invoke_ptr_ = other.invoke_ptr_;
            this->destroy_ptr_ = other.destroy_ptr_;
            this->copy_ptr_ = other.copy_ptr_;
            this->move_ptr_ = other.move_ptr_;
            if (this->copy_ptr_)
            {
                this->copy_ptr_(this->storage_, other.storage_);
            }
        }
        return *this;
    }

    /**
     * @brief Move-assigns from another wrapper.
     * @param other Source wrapper.
     * @return This wrapper.
     * @note The current target, if any, is destroyed before the new one is moved in. The source becomes empty.
     */
    function& operator=(function&& other) CASTLE_NOEXCEPT
    {
        if (this != &other)
        {
            if (this->destroy_ptr_)
            {
                this->destroy_ptr_(this->storage_);
            }
            this->invoke_ptr_ = other.invoke_ptr_;
            this->destroy_ptr_ = other.destroy_ptr_;
            this->copy_ptr_ = other.copy_ptr_;
            this->move_ptr_ = other.move_ptr_;
            if (this->move_ptr_)
            {
                this->move_ptr_(this->storage_, other.storage_);
            }

            other.reset_pointers();
        }
        return *this;
    }

    /**
     * @brief Constructs the wrapper from a raw function pointer.
     * @param cb_ptr Function pointer to store.
     * @note Passing nullptr produces an empty wrapper.
     */
    function(callback_ptr_t cb_ptr) CASTLE_NOEXCEPT
    {
        if (cb_ptr != nullptr) // LCOV_EXCL_BR_LINE
        {
            static_assert(sizeof(callback_ptr_t) <= StorageSize,
                          "Callback pointer is too large for function storage");
            static_assert(StorageAlignment != 0U && (StorageAlignment & (StorageAlignment - 1U)) == 0U,
                          "StorageAlignment must be a non-zero power of two");

            new (storage_) callback_ptr_t(cb_ptr);

            invoke_ptr_ = [](void* storage, Args... args) -> R
            {
                callback_ptr_t cb = *static_cast<callback_ptr_t*>(storage);

                CASTLE_IF_CONSTEXPR (meta::is_void<R>::value)
                {
                    cb(CASTLE_FORWARD<Args>(args)...);
                }
                else
                {
                    return cb(CASTLE_FORWARD<Args>(args)...);
                }
            };

            destroy_ptr_ = [](void* storage)
            {
                static_cast<callback_ptr_t*>(storage)->~callback_ptr_t();
            };

            copy_ptr_ = [](void* target_storage, CASTLE_CONST void* source_storage)
            {
                new (target_storage) callback_ptr_t(*static_cast<CASTLE_CONST callback_ptr_t*>(source_storage));
            };

            move_ptr_ = [](void* target_storage, void* source_storage)
            {
                new (target_storage) callback_ptr_t(*static_cast<callback_ptr_t*>(source_storage));
                static_cast<callback_ptr_t*>(source_storage)->~callback_ptr_t();
            };
        }
    }

    /**
     * @brief Replaces the stored target with a raw function pointer.
     * @param cb_ptr Function pointer to store, or nullptr to clear the wrapper.
     * @return This wrapper.
     * @note The previous target, if any, is destroyed first.
     */
    function& operator=(callback_ptr_t cb_ptr) CASTLE_NOEXCEPT
    {
        if (this->destroy_ptr_) // LCOV_EXCL_BR_LINE
        {
            this->destroy_ptr_(this->storage_);
        }

        if (cb_ptr != nullptr)
        {
            static_assert(sizeof(callback_ptr_t) <= StorageSize,
                          "Callback pointer is too large for function storage");
            static_assert(StorageAlignment != 0U && (StorageAlignment & (StorageAlignment - 1U)) == 0U,
                          "StorageAlignment must be a non-zero power of two");

            new (storage_) callback_ptr_t(cb_ptr);

            invoke_ptr_ = [](void* storage, Args... args) -> R
            {
                callback_ptr_t cb = *static_cast<callback_ptr_t*>(storage);

                CASTLE_IF_CONSTEXPR (meta::is_void<R>::value)
                {
                    cb(CASTLE_FORWARD<Args>(args)...);
                }
                else
                {
                    return cb(CASTLE_FORWARD<Args>(args)...);
                }
            };

            destroy_ptr_ = [](void* storage)
            {
                static_cast<callback_ptr_t*>(storage)->~callback_ptr_t();
            };

            copy_ptr_ = [](void* target_storage, CASTLE_CONST void* source_storage)
            {
                new (target_storage) callback_ptr_t(*static_cast<CASTLE_CONST callback_ptr_t*>(source_storage));
            };

            move_ptr_ = [](void* target_storage,
                        void* source_storage)
            {
                new (target_storage) callback_ptr_t(*static_cast<callback_ptr_t*>(source_storage));
                static_cast<callback_ptr_t*>(source_storage)->~callback_ptr_t();
            };
        }
        else
        {
            reset_pointers();
        }

        return *this;
    }

    /**
     * @brief Destroys the stored callable if the wrapper is non-empty.
     */
    ~function()
    {
        if (destroy_ptr_)
        {
            destroy_ptr_(storage_);
        }
    }

    /**
     * @brief Reports whether a callable target is currently stored.
     * @return True when operator() can be called safely.
     */
    explicit operator bool() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return this->invoke_ptr_ != nullptr;
    }

    /**
     * @brief Invokes the stored callable.
     * @param args Arguments forwarded to the target.
     * @return The target's return value when `R` is non-void.
     * @note An empty wrapper triggers CASTLE_ASSERT according to the configured Castle assertion policy.
     */
    R operator()(Args... args) CASTLE_CONST
    {
        CASTLE_ASSERT(this->invoke_ptr_ != nullptr, // LCOV_EXCL_BR_LINE
                      "Attempting to invoke an empty function");

        CASTLE_IF_CONSTEXPR (meta::is_void<R>::value)
        {
            this->invoke_ptr_(
                const_cast<void*>(static_cast<CASTLE_CONST void*>(storage_)),
                CASTLE_FORWARD<Args>(args)...
            );
        }
        else
        {
            return this->invoke_ptr_(
                const_cast<void*>(static_cast<CASTLE_CONST void*>(storage_)),
                CASTLE_FORWARD<Args>(args)...
            );
        }
    }

private:
    /**
     * @brief Clears the function-pointer table so the wrapper becomes empty.
     */
    void reset_pointers() CASTLE_NOEXCEPT
    {
        invoke_ptr_ = nullptr;
        destroy_ptr_ = nullptr;
        copy_ptr_ = nullptr;
        move_ptr_ = nullptr;
    }

    alignas(StorageAlignment) uint8_t storage_[StorageSize];

    invoke_ptr_t invoke_ptr_ = nullptr;
    destroy_ptr_t destroy_ptr_ = nullptr;
    copy_ptr_t copy_ptr_ = nullptr;
    move_ptr_t move_ptr_ = nullptr;
};

} // namespace callbacks
} // namespace castle

#endif // CASTLE_CALLBACKS_FUNCTION_HPP
