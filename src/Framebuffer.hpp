#ifndef SIMPLE_GAME_TEMPLATE_FRAMEBUFFER_HPP
#define SIMPLE_GAME_TEMPLATE_FRAMEBUFFER_HPP

#include <stdint.h>
#include "Box2.hpp"

struct Framebuffer
{
    uint32_t width;
    uint32_t height;
    int pitch;
    uint8_t *pixels;

    Vector2I extent() const
    {
        return Vector2I(width, height);
    }

    Box2I bounds() const
    {
        return Box2I(Vector2I::zeros(), extent());
    }
};

#endif //SIMPLE_GAME_TEMPLATE_FRAMEBUFFER_HPP
