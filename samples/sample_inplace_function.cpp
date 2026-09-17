#include "castle/callbacks/inplace_function.h"

#include <iostream>
#include <string>
#include <utility>

using namespace castle::callbacks;

//=====================================================
// Free function
//=====================================================
void free_function(int value)
{
    std::cout << "free_function            : " << value << std::endl;
}

//=====================================================
// Functor
//=====================================================
struct Functor
{
    int base = 1000;

    void operator()(int value) const
    {
        std::cout << "Functor                  : " << (base + value) << std::endl;
    }
};

//=====================================================
// Handler class (member functions)
//=====================================================
class Handler
{
public:
    void member_function(int value)
    {
        std::cout << "member_function          : " << value << std::endl;
    }

    void member_function_const(int value) const
    {
        std::cout << "member_function_const    : " << value << std::endl;
    }
};

//=====================================================
// Non-void return type demo helper
//=====================================================
static int add(int a, int b)
{
    return a + b;
}

int main()
{
    std::cout << "==============================" << std::endl;
    std::cout << "inplace_function - free function" << std::endl;
    std::cout << "==============================" << std::endl;

    using signature_t = void(int);

    inplace_function<signature_t> cb_free(&free_function);
    cb_free(1);

    //-----------------------------------------------------------------

    std::cout << "\n==============================" << std::endl;
    std::cout << "inplace_function - functor (stored by value)" << std::endl;
    std::cout << "==============================" << std::endl;

    Functor functor;
    inplace_function<signature_t> cb_functor(functor);
    cb_functor(2);

    //-----------------------------------------------------------------

    std::cout << "\n==============================" << std::endl;
    std::cout << "inplace_function - lambda with capture" << std::endl;
    std::cout << "==============================" << std::endl;

    int offset = 100;
    inplace_function<signature_t> cb_lambda(
        [offset](int value)
        {
            std::cout << "lambda                   : " << (value + offset) << std::endl;
        }
    );
    cb_lambda(3);

    //-----------------------------------------------------------------

    std::cout << "\n==============================" << std::endl;
    std::cout << "inplace_function - bound member function via lambda" << std::endl;
    std::cout << "==============================" << std::endl;

    Handler handler;
    inplace_function<signature_t> cb_member(
        [&handler](int value)
        {
            handler.member_function(value);
        }
    );
    cb_member(4);

    const Handler const_handler;
    inplace_function<signature_t> cb_member_const(
        [&const_handler](int value)
        {
            const_handler.member_function_const(value);
        }
    );
    cb_member_const(5);

    //-----------------------------------------------------------------

    std::cout << "\n==============================" << std::endl;
    std::cout << "inplace_function - copy semantics" << std::endl;
    std::cout << "==============================" << std::endl;

    inplace_function<signature_t> cb_copy = cb_lambda; // copy-construct
    cb_copy(6);

    inplace_function<signature_t> cb_copy_assign;
    cb_copy_assign = cb_functor;                       // copy-assign
    cb_copy_assign(7);

    //-----------------------------------------------------------------

    std::cout << "\n==============================" << std::endl;
    std::cout << "inplace_function - move semantics" << std::endl;
    std::cout << "==============================" << std::endl;

    inplace_function<signature_t> cb_source(
        [](int value)
        {
            std::cout << "moved lambda             : " << value << std::endl;
        }
    );

    inplace_function<signature_t> cb_moved(std::move(cb_source));
    cb_moved(8);
    std::cout << "cb_source valid?         : " << static_cast<bool>(cb_source) << std::endl;

    inplace_function<signature_t> cb_move_assign;
    cb_move_assign = std::move(cb_moved);
    cb_move_assign(9);

    //-----------------------------------------------------------------

    std::cout << "\n==============================" << std::endl;
    std::cout << "inplace_function - empty state & operator bool" << std::endl;
    std::cout << "==============================" << std::endl;

    inplace_function<signature_t> cb_empty;
    std::cout << "cb_empty valid?          : " << static_cast<bool>(cb_empty) << std::endl;
    cb_empty = &free_function;
    std::cout << "after assign valid?      : " << static_cast<bool>(cb_empty) << std::endl;
    cb_empty(10);

    //-----------------------------------------------------------------

    std::cout << "\n==============================" << std::endl;
    std::cout << "inplace_function - non-void return type" << std::endl;
    std::cout << "==============================" << std::endl;

    inplace_function<int(int, int)> cb_add(&add);
    std::cout << "add(11, 22)              : " << cb_add(11, 22) << std::endl;

    //-----------------------------------------------------------------

    std::cout << "\n==============================" << std::endl;
    std::cout << "inplace_function - custom storage size" << std::endl;
    std::cout << "==============================" << std::endl;

    // Larger inline storage to hold a heavier capture.
    std::string tag = "heavy-capture";
    inplace_function<signature_t, 128> cb_big(
        [tag](int value)
        {
            std::cout << "big lambda [" << tag << "] : " << value << std::endl;
        }
    );
    cb_big(12);

    return 0;
}