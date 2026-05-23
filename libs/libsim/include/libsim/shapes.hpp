#ifndef LIBGRID_SHAPES_HPP
#define LIBGRID_SHAPES_HPP

#include "libsim/types.hpp"

#include <numbers>

namespace grid::libsim {
 
    template<typename Coord>
    class Shape {
    public:
        Shape(const Coord& centerCoord) : center(centerCoord) {}
        virtual ~Shape() = default;
        virtual double area() const = 0;
        virtual double perimeter() const = 0;
        virtual bool containsPoint(const Coord& point) const = 0;
    protected:
        Coord center;
    };

    template<typename Coord>
    class Circle : public Shape<Coord> {
        
        double radius;
    public:
        Circle(const Coord& centerCoord, double radius) : Shape<Coord>(centerCoord), radius(radius) {}
        
        bool containsPoint(const Coord& point) const override {
            auto distance = this->center - point;
            return distance.magnitude() <= radius;
        }

        double area() const override {
            return std::numbers::pi * radius * radius;
        }

        double perimeter() const override {
            return 2 * std::numbers::pi * radius;
        }
    };

    template<typename Coord>
    class Rectangle : public Shape<Coord> {
        Coord halfExtents;
    public:
        Rectangle(const Coord& centerCoord, const Coord& halfExtents)
            : Shape<Coord>(centerCoord), halfExtents(halfExtents) {}

        bool containsPoint(const Coord& point) const override {
            for (size_t i = 0; i < Coord::num_dimensions; i++) {
                auto val = point[i] - this->center[i];
                if (val < -halfExtents[i] || val > halfExtents[i]) return false;
            }
            return true;
        }

        double area() const override {
            double result = 1.0;
            for (size_t i = 0; i < Coord::num_dimensions; i++) {
                result *= 2.0 * halfExtents[i];
            }
            return result;
        }

        double perimeter() const override {
            double sum = 0.0;
            for (size_t i = 0; i < Coord::num_dimensions; i++) {
                sum += 2.0 * halfExtents[i];
            }
            return 2.0 * sum;
        }
    };
}// namespace grid::libsim

#endif // LIBGRID_SHAPES_HPP
