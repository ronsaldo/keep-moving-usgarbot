#ifndef SIMPLE_GAME_TEMPLATE_FRAMEBUFFER_HPP
#define SIMPLE_GAME_TEMPLATE_FRAMEBUFFER_HPP

#include <stdint.h>

struct Framebuffer
{
    uint32_t width;
    uint32_t height;
    int pitch;
    uint8_t *pixels;
};

#endif //SIMPLE_GAME_TEMPLATE_FRAMEBUFFER_HPP
