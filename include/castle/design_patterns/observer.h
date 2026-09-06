#ifndef CASTLE_DESIGN_PATTERNS_OBSERVER_H
#define CASTLE_DESIGN_PATTERNS_OBSERVER_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"
#include "castle/core/types.h"
#include "castle/container/array.h"

#include <stddef.h>

namespace castle
{
namespace design_patterns
{

template <typename... Types>
class observer;

template <typename T>
class observer<T>
{
public:
    virtual ~observer() CASTLE_DEFAULT;
    virtual void notify(CASTLE_CONST T& data) = 0;
};

template <>
class observer<void>
{
public:
    virtual ~observer() CASTLE_DEFAULT;
    virtual void notify() = 0;
};

template <typename T, typename... Rest>
class observer<T, Rest...> : public observer<T>, public observer<Rest...>
{
    static_assert(meta::has_unique_types<T, Rest...>::value,
                  "Observer types must be unique.");

public:
    using observer<T>::notify;
    using observer<Rest...>::notify;
};

template <typename TOberver, size_type N>
class observable
{
public:
    bool add_observer(TOberver* observer) CASTLE_NOEXCEPT
    {
        if (observer == nullptr)
        {
            return false;
        }

        if (observer_count_ >= N)
        {
            return false; // Maximum number of observers reached
        }

        size_type first_empty_slot = N;
        for (size_type i = 0; i < N; ++i)
        {
            if (observers_[i] == nullptr)
            {
                if (first_empty_slot == N)
                {
                    first_empty_slot = i;
                }
            }
            else if (observers_[i] == observer)
            {
                return false; // Observer already exists
            }
        }

        observers_[first_empty_slot] = observer;
        ++observer_count_;
        return true;
    }

    bool remove_observer(TOberver* observer) CASTLE_NOEXCEPT
    {
        if (observer == nullptr)
        {
            return false;
        }

        for (auto& obs : observers_)
        {
            if (obs == observer)
            {
                obs = nullptr;
                --observer_count_;
                return true;
            }
        }
        return false;
    }

    template <typename TObserverType>
    void notify_observers(CASTLE_CONST TObserverType& data) CASTLE_NOEXCEPT
    {
        for (CASTLE_CONST auto& observer : observers_)
        {
            if (observer != nullptr)
            {
                observer->notify(data);
            }
        }
    }

    void notify_observers() CASTLE_NOEXCEPT
    {
        for (CASTLE_CONST auto& observer : observers_)
        {
            if (observer != nullptr)
            {
                observer->notify();
            }
        }
    }

private:
    container::array<TOberver*, N> observers_{};
    size_type observer_count_ = 0U;
};

} // namespace design_patterns
} // namespace castle

#endif // CASTLE_DESIGN_PATTERNS_OBSERVER_H
