#ifndef SCALAR_HPP
#define SCALAR_HPP

#include <math.h>
#include <algorithm>

inline float sign(float x)
{
    return x > 0 ? 1 : (x < 0 ? -1 : 0);
}

inline float sawToothWave(float x)
{
    return (x - floor(x))*2.0f - 1.0f;
}

inline float sineWave(float x)
{
    return sin(x*M_PI*2.0f);
}

#endif //SCALAR_HPP
