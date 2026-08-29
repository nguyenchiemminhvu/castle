#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <new>       // placement new
#include <cassert>

namespace castle
{
namespace callbacks
{

template <typename Signature,
          std::size_t StorageSize = 64,
          std::size_t StorageAlignment = alignof(std::max_align_t)>
class inplace_function;

template <typename R,
          typename... Args,
          std::size_t StorageSize,
          std::size_t StorageAlignment>
class inplace_function<R(Args...), StorageSize, StorageAlignment>
{
public:
    using invoke_ptr_t = R(*)(void*, Args...);
    using destroy_ptr_t = void(*)(void*);
    using copy_ptr_t = void(*)(void*, const void*); // pointer to target and source
    using move_ptr_t = void(*)(void*, void*); // pointer to target and source

    inplace_function() = default;

    template <typename Callable,
              typename = std::enable_if_t<!std::is_same_v<std::decay_t<Callable>, inplace_function>>>
    inplace_function(Callable&& callable)
    {
        using decayed_callable = std::decay_t<Callable>;

        static_assert(sizeof(decayed_callable) <= StorageSize, "Callable is too large for inplace_function storage");
        static_assert(alignof(decayed_callable) <= StorageAlignment, "Callable has too strict alignment for inplace_function storage");

        new (storage_) decayed_callable(std::forward<Callable>(callable));

        invoke_ptr_ = [](void* storage, Args... args) -> R {
            return (*static_cast<decayed_callable*>(storage))(std::forward<Args>(args)...);
        };

        destroy_ptr_ = [](void* storage) {
            static_cast<decayed_callable*>(storage)->~decayed_callable();
        };

        copy_ptr_ = [](void* target_storage, const void* source_storage) {
            new (target_storage) decayed_callable(*static_cast<const decayed_callable*>(source_storage));
        };

        move_ptr_ = [](void* target_storage, void* source_storage) {
            new (target_storage) decayed_callable(std::move(*static_cast<decayed_callable*>(source_storage)));
            static_cast<decayed_callable*>(source_storage)->~decayed_callable();
        };
    }

    inplace_function(const inplace_function& other)
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

    inplace_function(inplace_function&& other) noexcept
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

    inplace_function& operator=(const inplace_function& other)
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

    inplace_function& operator=(inplace_function&& other) noexcept
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

    explicit operator bool() const noexcept
    {
        return this->invoke_ptr_ != nullptr;
    }

    R operator()(Args... args) const
    {
        assert(this->invoke_ptr_ != nullptr && "Attempting to invoke an empty inplace_function");

        if constexpr (std::is_void_v<R>)
        {
            this->invoke_ptr_(
                const_cast<void*>(static_cast<const void*>(storage_)),
                std::forward<Args>(args)...
            );
        }
        else
        {
            return this->invoke_ptr_(
                const_cast<void*>(static_cast<const void*>(storage_)),
                std::forward<Args>(args)...
            );
        }
    }

private:
    void reset_pointers() noexcept
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