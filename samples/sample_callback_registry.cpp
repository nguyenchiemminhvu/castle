#include "castle/callbacks/callback_registry.h"

#include <iostream>

using namespace castle::callbacks;

//=====================================================
// Free function
//=====================================================
void free_function(int value)
{
    std::cout << "free_function : " << value << std::endl;
}

//=====================================================
// Functor
//=====================================================
struct Functor
{
    void operator()(int value)
    {
        std::cout << "Functor       : " << value << std::endl;
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
        std::cout << "member_function : " << value << std::endl;
    }

    void member_function_const(int value) const
    {
        std::cout << "member_function_const : " << value << std::endl;
    }
};

//=====================================================
// Global object cho function_ct_im
//=====================================================
Handler g_handler;

int main()
{
    std::cout << "==============================" << std::endl;
    std::cout << "callback_registry" << std::endl;
    std::cout << "==============================" << std::endl;

    using callback_signature_t = void(int);
    callback_registry<4, callback_signature_t> registry;
    registry(0); // nothing happens, no callbacks registered

    function<void(int)> cb_func(&free_function);
    registry.subscribe(&cb_func)
    ? std::cout << "Subscribed free_function" << std::endl
    : std::cout << "Failed to subscribe free_function" << std::endl;

    Functor functor;
    function_f<Functor, void(int)> cb_functor(functor);
    registry.subscribe(&cb_functor)
    ? std::cout << "Subscribed Functor" << std::endl
    : std::cout << "Failed to subscribe Functor" << std::endl;

    function_m<Handler, void(int)> cb_member(g_handler, &Handler::member_function);
    registry.subscribe(&cb_member)
    ? std::cout << "Subscribed member_function" << std::endl
    : std::cout << "Failed to subscribe member_function" << std::endl;

    function_ct_im<g_handler, &Handler::member_function_const> cb_member_const;
    registry.subscribe(&cb_member_const)
    ? std::cout << "Subscribed member_function_const" << std::endl
    : std::cout << "Failed to subscribe member_function_const" << std::endl;

    function_ct_f<Functor, void(int)> cb_ct_functor;
    registry.subscribe(&cb_ct_functor)
    ? std::cout << "Subscribed cb_ct_functor" << std::endl
    : std::cout << "Failed to subscribe cb_ct_functor" << std::endl;

    registry(1); // invoke all registered callbacks with argument 1

    registry.clear(); // clear all registered callbacks

    registry(2); // nothing happens, all callbacks cleared

    return 0;
}