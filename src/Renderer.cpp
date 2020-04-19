#include "GameInterface.hpp"
#include "HostInterface.hpp"
#include "GameLogic.hpp"
#include "MapTransientState.hpp"
#include <algorithm>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>

class Renderer
{
public:
    Renderer(const Framebuffer &f)
        : framebuffer(f)
    {
        halfFramebufferOffset = f.extent().asVector2F()/2;
        framebufferUnitExtent = f.extent().asVector2F()*UnitsPerPixel;
        halfFramebufferUnitOffset = halfFramebufferOffset*Vector2F(UnitsPerPixel, -UnitsPerPixel);

        {
            // Break time.
            auto fbBounds = f.bounds();
            viewVolumeInUnits = Box2F::withCenterAndHalfExtent(fbBounds.center().asVector2F()*Vector2F(UnitsPerPixel, -UnitsPerPixel), fbBounds.halfExtent().asVector2F()*UnitsPerPixel);
        }
    }

    const Framebuffer &framebuffer;
    Box2F viewVolumeInUnits;
    Box2F worldViewVolumeInUnits;

    Vector2F cameraTranslation;
    Vector2I cameraPixelOffset;
    Vector2F halfFramebufferOffset;
    Vector2F framebufferUnitExtent;
    Vector2F halfFramebufferUnitOffset;

    void renderBackground()
    {
        auto destRow = framebuffer.pixels;
        for(uint32_t y = 0; y < framebuffer.height; ++y)
        {
            auto dest = reinterpret_cast<uint32_t*> (destRow);
            for(uint32_t x = 0; x < framebuffer.width; ++x)
            {
                auto px = int(x + global.currentTime*10.0);
                auto py = int(y);
                *dest++ = ((px & 4) ^ (py & 4)) != 0 ? 0xff707070 : 0xff505050;
            }

            destRow += framebuffer.pitch;
        }
    }

    Vector2F worldToViewPixels(const Vector2F &p) const
    {
        return pointFromWorldIntoPixelSpace(p + cameraTranslation);
    }

    Box2F worldToViewPixels(const Box2F &b) const
    {
        return boxFromWorldIntoPixelSpace(b.translatedBy(cameraTranslation));
    }

    void renderCurrentMap()
    {
        if(!global.mapTransientState)
            return;

        {
            auto mapExtent = global.currentMap->extent().asVector2F()*UnitsPerPixel;
            auto mapClippingExtent = Vector2F(std::max(mapExtent.x - framebufferUnitExtent.x, framebufferUnitExtent.x), mapExtent.y);

            auto cameraPosition = Vector2F(global.cameraPosition.x, global.cameraPosition.y) - halfFramebufferUnitOffset;
            cameraPosition = std::max(cameraPosition, Vector2F(0.0, framebufferUnitExtent.y));
            cameraPosition = std::min(cameraPosition, mapClippingExtent);

            cameraTranslation = -cameraPosition;
        }

        cameraPixelOffset = pointFromWorldIntoPixelSpace(cameraTranslation).floor().asVector2I();
        worldViewVolumeInUnits = viewVolumeInUnits.translatedBy(-cameraTranslation);

        for(auto layer : global.mapTransientState->layers)
        {
            switch(layer->type)
            {
            case MapLayerType::Solid:
                renderTileLayer(*reinterpret_cast<MapSolidLayerState*> (layer)->mapTileLayer);
                break;
            case MapLayerType::Entities:
                renderEntityLayer(reinterpret_cast<MapEntityLayerState*> (layer));
                break;
            default:
                break;
            }
        }
    }

    void renderEntityLayer(MapEntityLayerState *layer)
    {
        for(auto entity : layer->entities)
            entity->renderWith(*this);
    }

    void render()
    {
        renderBackground();
        renderCurrentMap();
    }

    void renderTileLayer(const MapFileTileLayer &layer)
    {
        auto &tileSet = global.mainTileSet;
        auto tileExtent = tileSet.tileExtent;
        auto layerExtent = layer.extent;
        auto layerOffset = Vector2I(0, -layerExtent.y*tileExtent.y);

        auto viewVolumeInTileSpace = layer.boxFromWorldIntoTileSpace(worldViewVolumeInUnits, tileExtent.asVector2F());
        auto tileGridBounds = viewVolumeInTileSpace.asBoundingIntegerBox().intersectionWithBox(layer.tileGridBounds());

        auto sourceRow = layer.tiles + tileGridBounds.min.y*layerExtent.x + tileGridBounds.min.x;
        for(int32_t ly = tileGridBounds.min.y; ly < tileGridBounds.max.y; ++ly)
        {
            auto source = sourceRow;
            for(int32_t lx = tileGridBounds.min.x; lx < tileGridBounds.max.x; ++lx)
            {
                auto tileIndex = *source;
                if(tileIndex > 0)
                {
                    Vector2I tileGridIndex;
                    if(tileSet.computeTileColumnAndRowFromIndex(tileIndex - 1, &tileGridIndex))
                    {
                        blitTile(tileSet, tileGridIndex, layerOffset + Vector2I(lx, ly)*tileExtent + cameraPixelOffset );
                    }
                }

                ++source;
            }

            sourceRow += layerExtent.x;
        }
    }

    void blitTile(const TileSet &tileSet, const Vector2I &tileGridIndex, const Vector2I &destination, bool flipX = false, bool flipY = false)
    {
        blitImage(tileSet.image, Box2I::withMinAndExtent(tileSet.tileExtent*tileGridIndex, tileSet.tileExtent), destination, flipX, flipY);
    }

    void blitTextTile(const TileSet &tileSet, const Vector2I &tileGridIndex, const Vector2I &destination, uint32_t color, bool flipX = false, bool flipY = false)
    {
        blitTextImage(tileSet.image, Box2I::withMinAndExtent(tileSet.tileExtent*tileGridIndex, tileSet.tileExtent), destination, color, flipX, flipY);
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
            sourceRow += (sourceRectangle.min.x + (extent.x - copyOffset.x - 1))*4;
        else
            sourceRow += clippedSource.min.x*4;

        if(flipY)
            sourceRow += image->pitch*(sourceRectangle.min.y + (extent.y - copyOffset.y - 1));
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

    void blitTextImage(const ImagePtr &image, const Box2I &sourceRectangle, const Vector2I &destination, uint32_t textColor, bool flipX = false, bool flipY = false)
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
            sourceRow += (sourceRectangle.min.x + (extent.x - copyOffset.x - 1))*4;
        else
            sourceRow += clippedSource.min.x*4;

        if(flipY)
            sourceRow += image->pitch*(sourceRectangle.min.y + (extent.y - copyOffset.y - 1));
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
                    *dest = textColor;
                ++dest;
                source += sourceRowIncrement;
            }

            destRow += framebuffer.pitch;
            sourceRow += sourcePitch;
        }
    }

    void fillWorldRectangle(const Box2F &rectangle, uint32_t color)
    {
        fillRectangle(worldToViewPixels(rectangle).asBox2I(), color);
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
    if(self->spriteSheet)
    {
        auto spriteOffset = self->spriteSheet->tileExtent.asVector2F()*0.5f;
        auto spriteDestination = (renderer.worldToViewPixels(self->position + self->spriteOffset) - spriteOffset).asVector2I();

        if(self->isInvincible())
            renderer.blitTextTile(*self->spriteSheet, self->spriteIndex, spriteDestination, 0xffffffff, self->spriteFlipX, self->spriteFlipY);
        else
            renderer.blitTile(*self->spriteSheet, self->spriteIndex, spriteDestination, self->spriteFlipX, self->spriteFlipY);
    }
    else
    {
        auto color = self->color;
        if(self->isInvincible())
            color = 0xffffffff;
        renderer.fillWorldRectangle(self->boundingBox(), color);
    }
}
