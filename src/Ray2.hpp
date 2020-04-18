#ifndef RAY2_HPP
#define RAY2_HPP

#include "Vector2.hpp"
#include <utility>

class Ray2F
{
public:
    typedef std::pair<bool, float> IntersectionResult;

    Vector2F origin;
    Vector2F direction;
    Vector2F inverseDirection;
    float minT;
    float maxT;

    Ray2F() = default;
    Ray2F(const Vector2F &corigin, const Vector2F &cdirection, float cminT = 0.0f, float cmaxT = INFINITY)
        : origin(corigin), direction(cdirection), inverseDirection(1.0f/cdirection), minT(cminT), maxT(cmaxT) {}

    static Ray2F fromSegment(const Vector2F &start, const Vector2F &end)
    {
        auto direction = end-start;
        return Ray2F(start, direction.normalized(), 0.0f, direction.length());
    }

    Vector2F pointAtT(float t) const
    {
        return origin + t*direction;
    }

    bool isValidT(float t) const
    {
        return minT <= t && t <= maxT;
    }

    Vector2F startPoint() const
    {
        return pointAtT(minT);
    }

    Vector2F endPoint() const
    {
        return pointAtT(maxT);
    }

};

#endif //RAY2
