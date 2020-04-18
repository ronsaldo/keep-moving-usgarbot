#ifndef TILE_SET_HPP
#define TILE_SET_HPP

#include "Image.hpp"
#include "HostInterface.hpp"
#include "Vector2.hpp"

class TileSet
{
public:
    void loadFrom(const char *path, uint32_t tw = 32, uint32_t th = 32);

    ImagePtr image;
    Vector2I tileExtent;
    Vector2I gridExtent;

    bool computeTileColumnAndRowFromIndex(uint32_t tileIndex, Vector2I *outTileGridIndex)
    {
        *outTileGridIndex = Vector2I(tileIndex % gridExtent.x, tileIndex / gridExtent.x);

        return outTileGridIndex->x < gridExtent.x && outTileGridIndex->y < gridExtent.y;
    }
};

#endif //TILE_SET_HPP
