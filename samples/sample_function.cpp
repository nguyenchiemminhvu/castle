#include "castle/callbacks/function.h"

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
    std::cout << "function (runtime free func)" << std::endl;
    std::cout << "==============================" << std::endl;

    using callback_signature_t = void(int);

    function<callback_signature_t> cb_func(&free_function);
    cb_func(1);

    //-----------------------------------------------------------------

    std::cout << "\n==============================" << std::endl;
    std::cout << "function_f (runtime functor)" << std::endl;
    std::cout << "==============================" << std::endl;

    Functor functor;
    function_f<Functor, callback_signature_t> cb_functor(functor);
    cb_functor(2);

    //-----------------------------------------------------------------

    std::cout << "\n==============================" << std::endl;
    std::cout << "function_f (lambda)" << std::endl;
    std::cout << "==============================" << std::endl;

    int offset = 100;
    auto lambda = [offset](int value)
    {
        std::cout << "lambda        : " << (value + offset) << std::endl;
    };
    function_f<decltype(lambda), callback_signature_t> cb_lambda(lambda);
    cb_lambda(3);

    //-----------------------------------------------------------------

    std::cout << "\n==============================" << std::endl;
    std::cout << "make_function_f" << std::endl;
    std::cout << "==============================" << std::endl;

    auto cb_make = make_function_f<callback_signature_t>(
        [offset](int value)
        {
            std::cout << "make_function_f : " << value << std::endl;
        }
    );
    cb_make(4);

    //-----------------------------------------------------------------

    std::cout << "\n==============================" << std::endl;
    std::cout << "function_fr (reference functor)" << std::endl;
    std::cout << "==============================" << std::endl;

    function_fr<Functor, callback_signature_t> cb_functor_ref(functor);
    cb_functor_ref(5);

    //-----------------------------------------------------------------

    std::cout << "\n==============================" << std::endl;
    std::cout << "function_m (member function)" << std::endl;
    std::cout << "==============================" << std::endl;

    Handler handler;
    function_m<Handler, callback_signature_t> cb_member(handler, &Handler::member_function);
    cb_member(6);

    //-----------------------------------------------------------------

    std::cout << "\n==============================" << std::endl;
    std::cout << "function_ct (compile-time free function)" << std::endl;
    std::cout << "==============================" << std::endl;

    function_ct<&free_function> cb_ct;
    cb_ct(7);

    //-----------------------------------------------------------------

    std::cout << "\n==============================" << std::endl;
    std::cout << "function_ct_f (compile-time functor)" << std::endl;
    std::cout << "==============================" << std::endl;

    function_ct_f<Functor, callback_signature_t> cb_ct_functor;
    cb_ct_functor(8);

    //-----------------------------------------------------------------

    std::cout << "\n==============================" << std::endl;
    std::cout << "function_ct_m (compile-time member)" << std::endl;
    std::cout << "==============================" << std::endl;

    function_ct_m<&Handler::member_function> cb_ct_member(handler);
    cb_ct_member(9);

    //-----------------------------------------------------------------

    std::cout << "\n==============================" << std::endl;
    std::cout << "function_ct_m (const member)" << std::endl;
    std::cout << "==============================" << std::endl;

    const Handler const_handler;
    function_ct_m<&Handler::member_function_const> cb_ct_member_const(const_handler);
    cb_ct_member_const(10);

    //-----------------------------------------------------------------

    std::cout << "\n==============================" << std::endl;
    std::cout << "function_ct_im (global instance)" << std::endl;
    std::cout << "==============================" << std::endl;

    function_ct_im<g_handler, &Handler::member_function> cb_ct_im;
    cb_ct_im(11);

    return 0;
}