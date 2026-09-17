// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @brief Fixed-capacity observer interfaces and observable registry helpers.
 *
 * Use this header when one object must notify a bounded set of listeners without heap
 * allocation. Observer registration stores raw pointers in inline storage; the caller is
 * responsible for lifetime management. Notification uses virtual dispatch but does not
 * require RTTI or STL containers.
 *
 * @code
 * #include "castle/design_patterns/observer.hpp"
 *
 * struct temperature_observer : castle::design_patterns::observer<unsigned>
 * {
 *     void notify(unsigned const& value) override { last = value; }
 *     unsigned last = 0U;
 * };
 * @endcode
 */
#ifndef CASTLE_DESIGN_PATTERNS_OBSERVER_HPP
#define CASTLE_DESIGN_PATTERNS_OBSERVER_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/container/array.hpp"

#include <stddef.h>

namespace castle
{
namespace design_patterns
{

template <typename... Types>
class observer;

/**
 * @brief Abstract observer interface for one payload type.
 *
 * @tparam T Notification payload type.
 */
template <typename T>
class observer<T>
{
public:
    /** @brief Destroys the observer interface. */
    virtual ~observer() CASTLE_DEFAULT;

    /**
     * @brief Receives a notification payload.
     *
     * @param data Payload supplied by the observable.
     */
    virtual void notify(CASTLE_CONST T& data) = 0;
};

/**
 * @brief Abstract observer interface for parameterless notifications.
 */
template <>
class observer<void>
{
public:
    /** @brief Destroys the observer interface. */
    virtual ~observer() CASTLE_DEFAULT;

    /**
     * @brief Receives a notification without payload data.
     */
    virtual void notify() = 0;
};

/**
 * @brief Combines several `notify()` overload requirements into one observer interface.
 *
 * @tparam T First payload type.
 * @tparam Rest Remaining unique payload types.
 */
template <typename T, typename... Rest>
class observer<T, Rest...> : public observer<T>, public observer<Rest...>
{
    static_assert(meta::has_unique_types<T, Rest...>::value,
                  "Observer types must be unique.");

public:
    using observer<T>::notify;
    using observer<Rest...>::notify;
};

/**
 * @brief Stores and notifies up to `N` observers of one concrete observer interface type.
 *
 * @tparam TOberver Observer interface or concrete observer type.
 * @tparam N Maximum number of stored observers.
 */
template <typename TOberver, size_type N>
class observable
{
public:
    /**
     * @brief Registers an observer pointer.
     *
     * @param observer Observer to add.
     * @return `true` on success; `false` if the pointer is null, already registered, or capacity is exhausted.
     */
    bool add_observer(TOberver* observer) CASTLE_NOEXCEPT
    {
        if (observer == nullptr)
        {
            return false;
        }

        if (observer_count_ >= N)
        {
            return false;
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
                return false;
            }
        }

        observers_[first_empty_slot] = observer;
        ++observer_count_;
        return true;
    }

    /**
     * @brief Unregisters an observer pointer.
     *
     * @param observer Observer to remove.
     * @return `true` when the observer was present and removed; otherwise `false`.
     */
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

    /**
     * @brief Notifies all registered observers with a payload.
     *
     * @tparam TObserverType Payload type accepted by the observer interface.
     * @param data Payload forwarded to each observer.
     */
    template <typename TObserverType>
    void notify_observers(CASTLE_CONST TObserverType& data) CASTLE_NOEXCEPT
    {
        for (CASTLE_CONST auto& observer : observers_)
        {
            if (observer != nullptr) // LCOV_EXCL_BR_LINE
            {
                observer->notify(data);
            }
        }
    }

    /**
     * @brief Notifies all registered observers without payload data.
     */
    void notify_observers() CASTLE_NOEXCEPT
    {
        for (CASTLE_CONST auto& observer : observers_)
        {
            if (observer != nullptr) // LCOV_EXCL_BR_LINE
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

#endif // CASTLE_DESIGN_PATTERNS_OBSERVER_HPP
