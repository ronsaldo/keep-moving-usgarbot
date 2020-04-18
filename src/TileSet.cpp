#include "TileSet.hpp"
#include "GameLogic.hpp"
#include "HostInterface.hpp"

void TileSet::loadFrom(const char *path, uint32_t tw, uint32_t th)
{
    image.reset(hostInterface->loadImage(path));
    tileExtent = Vector2I(tw, th);
    gridExtent = Vector2I(image->width, image->height) / tileExtent;
}
