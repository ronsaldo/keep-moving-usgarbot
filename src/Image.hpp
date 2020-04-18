#ifndef SIMPLE_GAME_TEMPLATE_IMAGE_HPP
#define SIMPLE_GAME_TEMPLATE_IMAGE_HPP

#include <stdint.h>
#include <memory>

class Image
{
public:
    Image()
        : width(0), height(0), pitch(0), bpp(0)
    {}

    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bpp;
    std::unique_ptr<uint8_t[]> data;
};

typedef std::unique_ptr<Image> ImagePtr;

#endif //SIMPLE_GAME_TEMPLATE_IMAGE_HPP
