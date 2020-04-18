#ifndef TILE_SET_HPP
#define TILE_SET_HPP

#include "Image.hpp"
#include "HostInterface.hpp"

class TileSet
{
public:
    void loadFrom(const char *path, uint32_t tw = 32, uint32_t th = 32);

    ImagePtr image;
    uint32_t tileWidth;
    uint32_t tileHeight;
};

#endif //TILE_SET_HPP
