#include "GameInterface.hpp"
#include "HostInterface.hpp"
#include "GameLogic.hpp"
#include "MapTransientState.hpp"

class Renderer
{
public:
    Renderer(const Framebuffer &f)
        : framebuffer(f) {}

    const Framebuffer &framebuffer;
    Vector2F cameraTranslation;
    Vector2I cameraPixelOffset;

    void renderBackground();
    void renderCurrentMap();

    void renderEntityLayer(MapEntityLayerState *layer)
    {
        for(auto entity : layer->entities)
            renderEntity(entity);
    }

    void renderEntity(Entity *entity)
    {
        auto entityClass = entityBehaviorTypeIntoClass(entity->type);
        entityClass->renderWith(entity, *this);
    }

    void render()
    {
        renderBackground();
        renderCurrentMap();
    }

    void renderTileLayer(const MapFileTileLayer &layer)
    {
        auto &tileSet = global.mainTileSet;

        auto source = layer.tiles;
        for(int ly = 0; ly < layer.extent.y; ++ly)
        {
            for(int lx = 0; lx < layer.extent.x; ++lx)
            {
                auto tileIndex = *source;
                if(tileIndex > 0)
                {
                    Vector2I tileGridIndex;
                    if(tileSet.computeTileColumnAndRowFromIndex(tileIndex - 1, &tileGridIndex))
                    {
                        blitTile(tileSet, tileGridIndex, Vector2I(lx, ly)*tileSet.tileExtent + cameraPixelOffset);
                    }
                }

                ++source;
            }
        }
    }

    void blitTile(const TileSet &tileSet, const Vector2I &tileGridIndex, const Vector2I &destination, bool flipX = false, bool flipY = false)
    {
        blitImage(tileSet.image, Box2I::withMinAndExtent(tileSet.tileExtent*tileGridIndex, tileSet.tileExtent), destination, flipX, flipY);
    }

    void blitImage(const ImagePtr &image, const Box2I &sourceRectangle, const Vector2I &destination, bool flipX = false, bool flipY = false)
    {
        auto extent = sourceRectangle.extent();
        auto destRectangle = Box2I::withMinAndExtent(destination, sourceRectangle.extent());
        auto clippedDest = destRectangle.intersectionWithBox(framebuffer.bounds());
        if(clippedDest.isEmpty())
            return;

        auto copyOffset = clippedDest.min - destination;
        auto clippedSource = Box2I(sourceRectangle.min + copyOffset, sourceRectangle.max);

        auto destRow = framebuffer.pixels + framebuffer.pitch*clippedDest.min.y + clippedDest.min.x*4;
        auto sourceRow = image->data.get();
        if(flipX)
            sourceRow += (extent.x - clippedSource.min.x - 1)*4;
        else
            sourceRow += clippedSource.min.x*4;

        if(flipY)
            sourceRow += image->pitch*(extent.y - clippedSource.min.y - 1);
        else
            sourceRow += image->pitch*clippedSource.min.y;

        auto sourceRowIncrement = flipX ? -1 : 1;
        auto sourcePitch = flipY ? int(-image->pitch) : int(image->pitch);

        for(int32_t y = clippedDest.min.y; y < clippedDest.max.y; ++y)
        {
            auto dest = reinterpret_cast<uint32_t*> (destRow);
            auto source = reinterpret_cast<uint32_t*> (sourceRow);
            for(int32_t x = clippedDest.min.x; x < clippedDest.max.x; ++x)
            {
                auto sourcePixel = *source;
                auto alpha = (sourcePixel >> 24) & 0xff;
                if(alpha > 0x80)
                    *dest = sourcePixel;
                ++dest;
                source += sourceRowIncrement;
            }

            destRow += framebuffer.pitch;
            sourceRow += sourcePitch;
        }

    }

    void fillRectangle(const Box2I &rectangle, uint32_t color)
    {
        auto clippedRectangle = rectangle.intersectionWithBox(framebuffer.bounds());
        if(clippedRectangle.isEmpty())
            return;

        auto alpha = (color >> 24) & 0xff;
        if(alpha < 0x80)
            return;

        auto destRow = framebuffer.pixels + framebuffer.pitch*clippedRectangle.min.y + clippedRectangle.min.x*4;
        for(int32_t y = clippedRectangle.min.y; y < clippedRectangle.max.y; ++y)
        {
            auto dest = reinterpret_cast<uint32_t*> (destRow);
            for(int32_t x = clippedRectangle.min.x; x < clippedRectangle.max.x; ++x)
            {
                *dest++ = color;
            }

            destRow += framebuffer.pitch;
        }

    }

};

void render(const Framebuffer &framebuffer)
{
    if(!global.isInitialized)
        return;

    Renderer r(framebuffer);
    r.render();
}

void EntityBehavior::renderWith(Entity *self, Renderer &renderer)
{
    auto bbox = self->boundingBox;
    auto renderBox = bbox.translatedBy(renderer.cameraTranslation).scaledCoordinatesBy(PixelsPerUnit).asBox2I();
    renderer.fillRectangle(renderBox, 0xff0000ff);
}
