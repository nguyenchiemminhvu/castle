#ifndef CASTLE_DESIGN_PATTERNS_SINGLETON_H
#define CASTLE_DESIGN_PATTERNS_SINGLETON_H

#include "castle/core/compiler.h"
#include "castle/core/error_handler.h"
#include "castle/core/traits.h"
#include "castle/core/types.h"
#include "castle/memory/alignment.h"
#include "castle/utility/utility.h"
#include "castle/memory/new.h"

//==============================================================================
// singleton.h
//
// A safety-critical, embedded-friendly singleton template.
//
// Key properties:
//   - No dynamic memory allocation (uses aligned in-class storage).
//   - No exceptions thrown; misuse is trapped via assertion.
//   - Deterministic construction/destruction under explicit user control.
//   - Non-copyable, non-movable, non-instantiable (all-static facade).
//   - Not thread-safe by design: the caller is responsible for synchronisation
//     around create()/destroy(). Read-only access via instance() after
//     construction is safe for concurrent readers.
//
// Usage:
//   class my_type;
//   using my_singleton = castle::design_patterns::singleton<my_type>;
//
//   bool valid = my_singleton::is_valid();     // false
//   // my_singleton::instance();               // asserts: not created
//   my_singleton::create(1, "hello world");    // forwarded to my_type ctor
//   my_type& ref = my_singleton::instance();   // ok
//   my_singleton::destroy();                   // dtor runs
//==============================================================================

namespace castle
{
namespace design_patterns
{

template <typename T>
class singleton
{
public:

    using value_type = T;

    //--------------------------------------------------------------------------
    // Construct the managed instance in-place using the provided arguments.
    // Precondition: is_valid() == false.
    //--------------------------------------------------------------------------
    template <typename... Args>
    static void create(Args&&... args)
    {
        CASTLE_ASSERT(!s_is_valid, "singleton_already_created");
        ::new (static_cast<void*>(&s_storage)) T(CASTLE_FORWARD<Args>(args)...);
        s_is_valid = true;
    }

    //--------------------------------------------------------------------------
    // Destroy the managed instance.
    // Precondition: is_valid() == true.
    //--------------------------------------------------------------------------
    static void destroy() noexcept(meta::is_nothrow_destructible<T>::value)
    {
        CASTLE_ASSERT(s_is_valid, "singleton_not_created");
        reinterpret_cast<T*>(&s_storage)->~T();
        s_is_valid = false;
    }

    //--------------------------------------------------------------------------
    // Retrieve a reference to the managed instance.
    // Precondition: is_valid() == true, otherwise asserts.
    //--------------------------------------------------------------------------
    static T& instance() noexcept
    {
        CASTLE_ASSERT(s_is_valid, "singleton_not_created");
        return *reinterpret_cast<T*>(&s_storage);
    }

    //--------------------------------------------------------------------------
    // Query whether the instance is currently constructed.
    //--------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
// Static member definitions. Header-only via templates: each T gets its own
// pair of storage/flag with zero-initialised state before start-up.
//------------------------------------------------------------------------------
template <typename T>
typename singleton<T>::storage_type singleton<T>::s_storage{};

template <typename T>
bool singleton<T>::s_is_valid = false;

} // namespace design_patterns
} // namespace castle

#endif // CASTLE_DESIGN_PATTERNS_SINGLETON_H
