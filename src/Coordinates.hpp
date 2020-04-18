#ifndef COORDINATES_HPP
#define COORDINATES_HPP

#include "Vector2.hpp"
#include "Box2.hpp"

#define PixelsPerUnit 32.0f
#define UnitsPerPixel (1.0f/PixelsPerUnit)

inline Vector2F pointFromWorldIntoPixelSpace(const Vector2F &point)
{
    return point * Vector2F(PixelsPerUnit, -PixelsPerUnit);
}

inline Vector2F vectorFromWorldIntoPixelSpace(const Vector2F &vector)
{
    return vector * PixelsPerUnit;
}

inline Box2F boxFromWorldIntoPixelSpace(const Box2F &box)
{
    return Box2F::withCenterAndHalfExtent(pointFromWorldIntoPixelSpace(box.center()), vectorFromWorldIntoPixelSpace(box.halfExtent()));
}

#endif //COORDINATES_HPP
