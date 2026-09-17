#include "castle/design_patterns/visitor.h"

#include <iostream>

using namespace castle::design_patterns;

class Circle;
class Square;
class Triangle;

class ShapeVisitor : public visitor<Circle&, Square&, Triangle&>
{
public:
    void visit(Circle& circle) override
    {
        (void)circle; // Suppress unused parameter warning
        std::cout << "Visiting Circle" << std::endl;
    }
    void visit(Square& square) override
    {
        (void)square; // Suppress unused parameter warning
        std::cout << "Visiting Square" << std::endl;
    }
    void visit(Triangle& triangle) override
    {
        (void)triangle; // Suppress unused parameter warning
        std::cout << "Visiting Triangle" << std::endl;
    }
};

class Circle : public visitable<ShapeVisitor>
{
public:
    void accept(ShapeVisitor& visitor) override
    {
        visitor.visit(*this);
    }
};

class Square : public visitable<ShapeVisitor>
{
public:
    void accept(ShapeVisitor& visitor) override
    {
        visitor.visit(*this);
    }
};

class Triangle : public visitable<ShapeVisitor>
{
public:
    void accept(ShapeVisitor& visitor) override
    {
        visitor.visit(*this);
    }
};

int main()
{
    Circle circle;
    Square square;
    Triangle triangle;

    ShapeVisitor visitor;
    circle.accept(visitor);
    square.accept(visitor);
    triangle.accept(visitor);

    return 0;
}