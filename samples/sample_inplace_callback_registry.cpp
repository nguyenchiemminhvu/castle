#include "castle/callbacks/inplace_callback_registry.h"

#include <iostream>
#include <utility>

using namespace castle::callbacks;

//=====================================================
// Free function
//=====================================================
void free_function(int value)
{
    std::cout << "  free_function        : " << value << std::endl;
}

//=====================================================
// Functor
//=====================================================
struct Functor
{
    void operator()(int value) const
    {
        std::cout << "  Functor              : " << value << std::endl;
    }
};

//=====================================================
// Handler class
//=====================================================
class Handler
{
public:
    void member_function(int value)
    {
        std::cout << "  member_function      : " << value << std::endl;
    }
};

static const char* to_string(callback_subscription_error e)
{
    switch (e)
    {
        case callback_subscription_error::ok:                   return "ok";
        case callback_subscription_error::full:                 return "full";
        case callback_subscription_error::invalid_callback:     return "invalid_callback";
        case callback_subscription_error::invalid_subscription: return "invalid_subscription";
    }
    return "?";
}

int main()
{
    using signature_t = void(int);
    using registry_t  = inplace_callback_registry<4, signature_t>;

    //-----------------------------------------------------------------

    std::cout << "==============================" << std::endl;
    std::cout << "inplace_callback_registry - subscribe & invoke" << std::endl;
    std::cout << "==============================" << std::endl;

    registry_t registry;
    registry(0); // nothing happens

    registry.subscribe(&free_function)
    ? std::cout << "subscribe free_function : ok" << std::endl
    : std::cout << "subscribe free_function : failed" << std::endl;

    Functor functor;
    auto sub_functor = registry.subscribe(functor);
    sub_functor
    ? std::cout << "subscribe functor       : ok" << std::endl
    : std::cout << "subscribe functor       : failed" << std::endl;

    Handler handler;
    registry.subscribe(
        [&handler](int value)
        {
            handler.member_function(value);
        }
    )
    ? std::cout << "subscribe member_function : ok" << std::endl
    : std::cout << "subscribe member_function : failed" << std::endl;

    int offset = 1000;
    auto sub_lambda = registry.subscribe(
        [offset](int value)
        {
            std::cout << "  lambda (+offset)     : " << (value + offset) << std::endl;
        }
    );
    sub_lambda
    ? std::cout << "subscribe lambda          : ok" << std::endl
    : std::cout << "subscribe lambda          : failed" << std::endl;

    std::cout << "-- invoke(1) --" << std::endl;
    registry(1);

    //-----------------------------------------------------------------

    std::cout << "\n==============================" << std::endl;
    std::cout << "inplace_callback_registry - unsubscribe" << std::endl;
    std::cout << "==============================" << std::endl;

    auto err = sub_functor.unsubscribe();
    std::cout << "unsubscribe functor  : " << to_string(err) << std::endl;

    // Double unsubscribe -> invalid_subscription (handle already reset).
    err = sub_functor.unsubscribe();
    std::cout << "unsubscribe again    : " << to_string(err) << std::endl;

    std::cout << "size() after remove  : " << registry.size() << std::endl;

    std::cout << "-- invoke(2) --" << std::endl;
    registry(2);

    //-----------------------------------------------------------------

    std::cout << "\n==============================" << std::endl;
    std::cout << "inplace_callback_registry - capacity boundary" << std::endl;
    std::cout << "==============================" << std::endl;

    // Currently active: free, member, lambda (3). Add one more to reach 4.
    auto sub_fill = registry.subscribe(
        [](int v)
        {
            std::cout << "  filler               : " << v << std::endl;
        }
    );
    sub_fill
    ? std::cout << "subscribe filler          : ok" << std::endl
    : std::cout << "subscribe filler          : failed" << std::endl;

    // Attempt one over capacity -> error::full.
    callback_subscription_error out_err{};
    auto sub_over = registry.subscribe([](int){}, &out_err);
    sub_over
    ? std::cout << "subscribe over-capacity   : ok" << std::endl
    : std::cout << "subscribe over-capacity   : failed" << " err = " << to_string(out_err) << std::endl;

    //-----------------------------------------------------------------

    std::cout << "\n==============================" << std::endl;
    std::cout << "inplace_callback_registry - stale handle after clear()" << std::endl;
    std::cout << "==============================" << std::endl;

    registry.clear();
    std::cout << "after clear size()   = " << registry.size() << std::endl;

    registry(3); // nothing fires

    // Old handle is now stale: generation was bumped by clear().
    err = sub_lambda.unsubscribe();
    std::cout << "stale unsubscribe    : " << to_string(err) << std::endl;

    //-----------------------------------------------------------------

    std::cout << "\n==============================" << std::endl;
    std::cout << "inplace_callback_registry - invalid callback" << std::endl;
    std::cout << "==============================" << std::endl;

    registry_t::callback_type empty_cb; // default-constructed, not engaged
    auto bad = registry.subscribe(std::move(empty_cb), &out_err);
    bad
    ? std::cout << "empty subscribe      : ok" << std::endl
    : std::cout << "empty subscribe      : failed" << " err=" << to_string(out_err) << std::endl;

    return 0;
}