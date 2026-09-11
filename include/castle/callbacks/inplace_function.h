#ifndef CASTLE_CALLBACKS_INPLACE_FUNCTION_H
#define CASTLE_CALLBACKS_INPLACE_FUNCTION_H

#include "castle/core/compiler.h"
#include "castle/core/config.h"
#include "castle/core/error_handler.h"
#include "castle/core/traits.h"
#include "castle/core/types.h"
#include "castle/utility/move.h"
#include "castle/utility/forward.h"
#include "castle/memory/new.h"

#include <stdint.h>

namespace castle
{
namespace callbacks
{

template <typename Signature,
          size_type StorageSize = castle::inplace_storage_reserved,
          size_type StorageAlignment = castle::inplace_alignment_default>
class inplace_function;

template <typename R,
          typename... Args,
          size_type StorageSize,
          size_type StorageAlignment>
class inplace_function<R(Args...), StorageSize, StorageAlignment>
{
public:
    using invoke_ptr_t = R(*)(void*, Args...);
    using destroy_ptr_t = void(*)(void*);
    using copy_ptr_t = void(*)(void*, CASTLE_CONST void*); // pointer to target and source
    using move_ptr_t = void(*)(void*, void*); // pointer to target and source

    inplace_function() CASTLE_DEFAULT;

    template <typename Callable,
          typename = meta::enable_if_t<
              !castle::is_same<
                  meta::decay_t<Callable>,
                  inplace_function
              >::value>>
    inplace_function(Callable&& callable)
    {
        using decayed_callable = castle::decay_t<Callable>;

        static_assert(sizeof(decayed_callable) <= StorageSize,
                      "Callable is too large for inplace_function storage");
        static_assert(StorageAlignment != 0U && (StorageAlignment & (StorageAlignment - 1U)) == 0U,
                      "StorageAlignment must be a non-zero power of two");

        new (storage_) decayed_callable(CASTLE_FORWARD<Callable>(callable));

        invoke_ptr_ = [](void* storage, Args... args) -> R {
            return (*static_cast<decayed_callable*>(storage))(CASTLE_FORWARD<Args>(args)...);
        };

        destroy_ptr_ = [](void* storage) {
            static_cast<decayed_callable*>(storage)->~decayed_callable();
        };

        copy_ptr_ = [](void* target_storage, CASTLE_CONST void* source_storage) {
            new (target_storage) decayed_callable(*static_cast<CASTLE_CONST decayed_callable*>(source_storage));
        };

        move_ptr_ = [](void* target_storage, void* source_storage) {
            new (target_storage) decayed_callable(CASTLE_MOVE(*static_cast<decayed_callable*>(source_storage)));
            static_cast<decayed_callable*>(source_storage)->~decayed_callable();
        };
    }

    inplace_function(CASTLE_CONST inplace_function& other)
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

    inplace_function(inplace_function&& other) CASTLE_NOEXCEPT
    {
        this->invoke_ptr_ = other.invoke_ptr_;
        this->destroy_ptr_ = other.destroy_ptr_;
        this->copy_ptr_ = other.copy_ptr_;
        this->move_ptr_ = other.move_ptr_;
        if (this->move_ptr_)
        {
            this->move_ptr_(this->storage_, other.storage_);
        }

        // move_ptr_ already destroyed other's object; empty it so its destructor is a no-op.
        other.reset_pointers();
    }

    inplace_function& operator=(CASTLE_CONST inplace_function& other)
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

    inplace_function& operator=(inplace_function&& other) CASTLE_NOEXCEPT
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

            // move_ptr_ already destroyed other's object; empty it so its destructor is a no-op.
            other.reset_pointers();
        }
        return *this;
    }

    ~inplace_function()
    {
        if (destroy_ptr_)
        {
            destroy_ptr_(storage_);
        }
    }

    explicit operator bool() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return this->invoke_ptr_ != nullptr;
    }

    R operator()(Args... args) CASTLE_CONST
    {
        CASTLE_ASSERT(this->invoke_ptr_ != nullptr,
                      "Attempting to invoke an empty inplace_function");

        CASTLE_IF_CONSTEXPR (castle::is_void<R>::value)
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

#endif // CASTLE_CALLBACKS_INPLACE_FUNCTION_H
