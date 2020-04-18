#ifndef VECTOR2_HPP
#define VECTOR2_HPP

#include <math.h>
#include <algorithm>

class Vector2I;
class Vector2F
{
public:
    float x, y;

    Vector2F() = default;

    Vector2F(float s)
        : x(s), y(s) {}

    Vector2F(float cx, float cy)
        : x(cx), y(cy) {}

    static Vector2F zeros()
    {
        return Vector2F(0.0f);
    }

    static Vector2F ones()
    {
        return Vector2F(1.0f);
    }

    static Vector2F infinity()
    {
        return Vector2F(INFINITY);
    }

    static Vector2F negativeInfinity()
    {
        return Vector2F(-INFINITY);
    }

    float dot(const Vector2F &o) const
    {
        return x*o.x + y*o.y;
    }

    float length2() const
    {
        return dot(*this);
    }

    float length() const
    {
        return sqrt(length2());
    }

    float cross(const Vector2F &o) const
    {
        return x*o.y - y*o.x;
    }

    Vector2F floor() const
    {
        return Vector2F(::floor(x), ::floor(y));
    }

    Vector2F ceil() const
    {
        return Vector2F(::ceil(x), ::ceil(y));
    }

    Vector2F asVector2F() const
    {
        return *this;
    }

    Vector2I asVector2I() const;

    const Vector2F &operator+=(const Vector2F &o)
    {
        *this = *this + o;
        return *this;
    }

    const Vector2F &operator-=(const Vector2F &o)
    {
        *this = *this + o;
        return *this;
    }

    Vector2F operator+(const Vector2F &o) const
    {
        return Vector2F(x + o.x, y + o.y);
    }

    Vector2F operator-(const Vector2F &o) const
    {
        return Vector2F(x - o.x, y - o.y);
    }

    Vector2F operator*(const Vector2F &o) const
    {
        return Vector2F(x * o.x, y * o.y);
    }

    Vector2F operator/(const Vector2F &o) const
    {
        return Vector2F(x / o.x, y / o.y);
    }

    Vector2F operator-() const
    {
        return Vector2F(-x, -y);
    }
};

class Vector2I
{
public:
    int32_t x, y;

    Vector2I() = default;

    Vector2I(int32_t s)
        : x(s), y(s) {}

    Vector2I(int32_t cx, int32_t cy)
        : x(cx), y(cy) {}

    static Vector2I zeros()
    {
        return Vector2I(0.0f);
    }

    static Vector2I ones()
    {
        return Vector2I(1.0f);
    }

    Vector2F asVector2F() const
    {
        return Vector2F(x, y);
    }

    Vector2I asVector2I() const
    {
        return *this;
    }

    int32_t dot(const Vector2I &o) const
    {
        return x*o.x + y*o.y;
    }

    int32_t cross(const Vector2I &o) const
    {
        return x*o.y - y*o.x;
    }

    Vector2I operator+(const Vector2I &o) const
    {
        return Vector2I(x + o.x, y + o.y);
    }

    Vector2I operator-(const Vector2I &o) const
    {
        return Vector2I(x - o.x, y - o.y);
    }

    Vector2I operator*(const Vector2I &o) const
    {
        return Vector2I(x * o.x, y * o.y);
    }

    Vector2I operator/(const Vector2I &o) const
    {
        return Vector2I(x / o.x, y / o.y);
    }

    Vector2I operator-() const
    {
        return Vector2I(-x, -y);
    }
};

inline Vector2I Vector2F::asVector2I() const
{
    return Vector2I(x, y);
}

namespace std
{
inline Vector2F max(const Vector2F &a, const Vector2F &b)
{
    return Vector2F(std::max(a.x, b.x), std::max(a.y, b.y));
}

inline Vector2F min(const Vector2F &a, const Vector2F &b)
{
    return Vector2F(std::min(a.x, b.x), std::min(a.y, b.y));
}

inline Vector2I max(const Vector2I &a, const Vector2I &b)
{
    return Vector2I(std::max(a.x, b.x), std::max(a.y, b.y));
}

inline Vector2I min(const Vector2I &a, const Vector2I &b)
{
    return Vector2I(std::min(a.x, b.x), std::min(a.y, b.y));
}
}
#endif //VECTOR2_HPP
