// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @brief Explicitly controlled singleton wrapper backed by aligned static storage.
 *
 * Use this header when exactly one instance of a type must exist and the caller needs
 * deterministic creation and destruction without heap allocation. Construction happens in
 * place through placement new, misuse is reported through Castle assertions, and the
 * caller owns all synchronization.
 *
 * @code
 * #include "castle/design_patterns/singleton.hpp"
 *
 * struct service
 * {
 *     explicit service(int seed) : value(seed) {}
 *     int value;
 * };
 *
 * using global_service = castle::design_patterns::singleton<service>;
 * @endcode
 */
#ifndef CASTLE_DESIGN_PATTERNS_SINGLETON_HPP
#define CASTLE_DESIGN_PATTERNS_SINGLETON_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/memory/alignment.hpp"
#include "castle/utility/utility.hpp"
#include "castle/memory/new.hpp"

namespace castle
{
namespace design_patterns
{

/**
 * @brief Stores one `T` instance in aligned static storage with explicit lifetime control.
 *
 * @tparam T Managed object type.
 *
 * @warning `create()` and `destroy()` are not thread-safe. Synchronize them externally
 * when multiple execution contexts may touch the singleton lifecycle.
 */
template <typename T>
class singleton
{
public:

    /** @brief Alias for the managed object type. */
    using value_type = T;

    /**
     * @brief Constructs the singleton instance in place.
     *
     * @tparam Args Constructor argument types.
     * @param args Constructor arguments forwarded to `T`.
     *
     * @note Asserts if the singleton already exists.
     */
    template <typename... Args>
    static void create(Args&&... args)
    {
        CASTLE_ASSERT(!s_is_valid, "singleton_already_created"); // LCOV_EXCL_BR_LINE
        ::new (static_cast<void*>(&s_storage)) T(CASTLE_FORWARD<Args>(args)...);
        s_is_valid = true;
    }

    /**
     * @brief Destroys the managed instance.
     *
     * @note Asserts if the singleton has not been created.
     */
    static void destroy() noexcept(meta::is_nothrow_destructible<T>::value)
    {
        CASTLE_ASSERT(s_is_valid, "singleton_not_created"); // LCOV_EXCL_BR_LINE
        reinterpret_cast<T*>(&s_storage)->~T();
        s_is_valid = false;
    }

    /**
     * @brief Returns a reference to the managed instance.
     *
     * @return Reference to the live singleton object.
     *
     * @note Asserts if the singleton has not been created.
     */
    static T& instance() noexcept
    {
        CASTLE_ASSERT(s_is_valid, "singleton_not_created"); // LCOV_EXCL_BR_LINE
        return *reinterpret_cast<T*>(&s_storage);
    }

    /**
     * @brief Reports whether the singleton currently contains a live object.
     *
     * @return `true` when `create()` has been called without a matching `destroy()`.
     */
    static bool is_valid() noexcept
    {
        return s_is_valid;
    }

private:

    singleton() CASTLE_DELETE;
    ~singleton() CASTLE_DELETE;
    singleton(CASTLE_CONST singleton&) CASTLE_DELETE;
    singleton& operator=(CASTLE_CONST singleton&) CASTLE_DELETE;
    singleton(singleton&&) CASTLE_DELETE;
    singleton& operator=(singleton&&) CASTLE_DELETE;

    using storage_type = typename memory::aligned_storage<sizeof(T), alignof(T)>::type;

    static storage_type s_storage;
    static bool         s_is_valid;
};

template <typename T>
typename singleton<T>::storage_type singleton<T>::s_storage{};

template <typename T>
bool singleton<T>::s_is_valid = false;

} // namespace design_patterns
} // namespace castle

#endif // CASTLE_DESIGN_PATTERNS_SINGLETON_HPP
