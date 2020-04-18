#include "Collisions.hpp"
#include "GameLogic.hpp"
#include "MapTransientState.hpp"

bool isWorldBoxCollidingWithMapTileLayer(const Box2F &box, const MapFileTileLayer *tileLayer)
{
    auto tileLayerExtent = tileLayer->extent;
    auto tileExtent = global.mainTileSet.tileExtent;
    auto boxInTileSpace = tileLayer->boxFromWorldIntoTileSpace(box, tileExtent.asVector2F());
    auto tileGridBox = boxInTileSpace.asBoundingIntegerBox().intersectionWithBox(tileLayer->tileGridBounds());

    auto sourceRow = tileLayer->tiles + tileGridBox.min.y*tileLayerExtent.x + tileGridBox.min.x;
    for(int32_t ty = tileGridBox.min.y; ty < tileGridBox.max.y; ++ty)
    {
        auto source = sourceRow;
        for(int32_t tx = tileGridBox.min.x; tx < tileGridBox.max.x; ++tx)
        {
            auto tileIndex = *source++;
            if(tileIndex == 0)
                continue;

            auto tileBox = Box2F(Vector2F(tx, ty), Vector2F(tx + 1, ty + 1));
            if(boxInTileSpace.intersectsWithBox(tileBox))
                return true;
        }

        sourceRow += tileLayerExtent.x;
    }
    return false;
}

bool isBoxCollidingWithWorld(const Box2F &box)
{
    auto mapState = global.mapTransientState;
    if(!mapState)
        return false;

    for(auto layer : mapState->layers)
    {
        if(layer->type != MapLayerType::Solid)
            continue;

        auto solidLayer = reinterpret_cast<MapSolidLayerState*> (layer);
        if(isWorldBoxCollidingWithMapTileLayer(box, solidLayer->mapTileLayer))
            return true;
    }
    return false;
}
