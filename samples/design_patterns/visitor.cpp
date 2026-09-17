#include "sample_support.hpp"

#include "castle/design_patterns/visitor.hpp"

struct start_command;
struct stop_command;
using command_visitor = castle::design_patterns::visitor<start_command&, stop_command&>;

struct start_command : castle::design_patterns::visitable<command_visitor>
{
    explicit start_command(int value) : argument(value) {}

    void accept(command_visitor& visitor) override
    {
        visitor.visit(*this);
    }

    int argument;
};

struct stop_command : castle::design_patterns::visitable<command_visitor>
{
    void accept(command_visitor& visitor) override
    {
        visitor.visit(*this);
    }
};

struct recorder : command_visitor
{
    void visit(start_command& command) override
    {
        last_argument = command.argument;
        stop_count = 0;
    }

    void visit(stop_command&) override
    {
        ++stop_count;
    }

    int last_argument = 0;
    int stop_count = 0;
};

int main()
{
    start_command start(42);
    stop_command stop;
    recorder visitor;

    start.accept(visitor);
    CASTLE_SAMPLE_CHECK(visitor.last_argument == 42);
    stop.accept(visitor);
    CASTLE_SAMPLE_CHECK(visitor.stop_count == 1);
    return 0;
}
