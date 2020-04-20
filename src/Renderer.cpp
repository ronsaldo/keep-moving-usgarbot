#include "GameInterface.hpp"
#include "HostInterface.hpp"
#include "GameLogic.hpp"
#include "MapTransientState.hpp"
#include <algorithm>
#include <vector>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>

static std::string CharacterSet = " ABCDEFGHIJKLMNOPQRSTUVWXYZ.,?!@0123456789^[]";
static uint32_t RainbowColorTable[] {
    0xff0000cc,
    0xff004fcc,
    0xff00cc00,

    0xffcc0000,
    0xff4fcc00,
    0xff00cccc,

    0xffcccc00,
    0xffcc004f,
    0xffcc00cc,

};
static const int RainbowColorTableSize = sizeof(RainbowColorTable)/sizeof(RainbowColorTable[0]);

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
        if(global.backgroundImage.get())
        {
            blitImage(global.backgroundImage, global.backgroundImage->bounds(), 0);
            return;
        }
        
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

    void drawCharacter(char character, const Vector2I &destPosition, uint32_t color)
    {
        if(character <= ' ')
            character = ' ';
        if('a' <= character && character <= 'z')
            character += 'A' - 'a';

        auto tileIndex = CharacterSet.find(character);
        if(tileIndex >= CharacterSet.size())
            return;

        Vector2I tileGridIndex;
        auto &tileSet = global.hudTiles;
        tileSet.computeTileColumnAndRowFromIndex(tileIndex, &tileGridIndex);

        auto tilePosition = tileGridIndex*tileSet.tileExtent;
        blitTextImage(tileSet.image, Box2I(tilePosition, tilePosition + tileSet.tileExtent), destPosition, color);
    }

    void drawString(const std::string &string, const Vector2I &destPosition, uint32_t color)
    {
        auto currentDestPosition = destPosition;
        for(auto c : string)
        {
            drawCharacter(c, currentDestPosition, color);
            currentDestPosition = currentDestPosition + Vector2I(global.hudTiles.tileExtent.x, 0);
        }
    }

    void drawRainbowString(const std::string &string, const Vector2I &destPosition, float wavePhase, size_t rainbowPhase = 0)
    {
        auto currentDestPosition = destPosition;

        for(size_t i = 0; i < string.size(); ++i)
        {
            auto waveOffset = (Vector2F(0.0f, sin(currentDestPosition.x*3.0f + wavePhase))*global.hudTiles.tileExtent.y*0.3f).floor().asVector2I();

            drawCharacter(string[i], waveOffset + currentDestPosition, RainbowColorTable[(i + rainbowPhase) % RainbowColorTableSize]);
            currentDestPosition = currentDestPosition + Vector2I(global.hudTiles.tileExtent.x, 0);
        }
    }

    uint32_t colorForHP(uint32_t hp)
    {
        if (hp >= 70)
            return 0xff00ff00;

        if (hp >= 30)
            return 0xff00ffff;
        if (hp > 0)
            return 0xff0000ff;

        return 0xff000000;
    }

    void renderHUD()
    {
        auto tileExtent = global.hudTiles.tileExtent;

        auto transientState = global.mapTransientState;
        if(!transientState)
            return;

        auto playerHP = transientState->activePlayer ? transientState->activePlayer->hitPoints : 0u;
        auto vipHP = transientState->activeVIP ? transientState->activeVIP->hitPoints : 0u;
        auto vipFollowing = transientState->isVipFollowingPlayer;

        auto hudOffset = Vector2I(20);
        {
            char buffer[64];
            sprintf(buffer, "@%03d", playerHP);
            drawString(buffer, hudOffset, colorForHP(playerHP));
        }

        {
            char buffer[64];
            sprintf(buffer, "^%03d", vipHP);
            drawString(buffer, hudOffset + Vector2I(0, tileExtent.y), colorForHP(vipHP));
        }

        if(vipHP > 0)
        {
            drawCharacter(vipFollowing ? ']' : '[', hudOffset + Vector2I(4*tileExtent.x, tileExtent.y), vipFollowing ? 0xff00cc00 : 0xff0000cc);
        }
    }

    void drawCenteredRainbowString(const std::string &string, float wavePhase, size_t rainbowPhase)
    {
        std::vector<std::string> lines;
        size_t lineStartIndex = 0;
        for(size_t i = 0; i < string.size(); ++i)
        {
            auto c = string[i];
            if(c == '\n')
            {
                lines.push_back(string.substr(lineStartIndex, i - lineStartIndex));
                lineStartIndex = i + 1;
            }
        }
        if(lineStartIndex <= string.size())
            lines.push_back(string.substr(lineStartIndex, string.size() - lineStartIndex));

        auto tileExtent = global.hudTiles.tileExtent.asVector2F();
        auto framebufferExtent = framebuffer.extent().asVector2F();
        auto verticalGap = tileExtent.y*2.0f;

        auto remainingHeight = framebufferExtent.y - tileExtent.y*lines.size() - std::max(verticalGap*(lines.size()-1), 0.0f);
        auto positionY = remainingHeight*0.5f;

        for(auto & line : lines)
        {
            auto remainingWidth = framebufferExtent.x - tileExtent.x*line.size();
            auto positionX = remainingWidth*0.5f;
            drawRainbowString(line, Vector2F(positionX, positionY).asVector2I(), wavePhase, rainbowPhase);
            positionY += tileExtent.y + verticalGap;
        }
    }

    Vector2I positionForCenteredStringOfSize(uint32_t size)
    {
        auto textExtent = global.hudTiles.tileExtent.asVector2F()*Vector2F(size, 1);
        auto remainingExtent = framebuffer.extent().asVector2F() - textExtent;

        return (remainingExtent*0.5f).asVector2I();
    }

    void renderActiveMessage()
    {
        auto transientState = global.mapTransientState;
        if(!transientState)
            return;

        if(transientState->currentMessageRemainingTime > 0.0f)
        {
            float wavePhase = -transientState->currentMessageRemainingTime*3.0f;
            size_t rainbowPhase = -transientState->currentMessageRemainingTime*3.0f;
            drawCenteredRainbowString(transientState->currentMessage, wavePhase, rainbowPhase);
        }
    }

    void renderGameStateMessage()
    {
        auto transientState = global.mapTransientState;

        if(global.isGameFinished)
        {
            drawGlobalRainbowMessage("Congratulations!!!.\n\nPress start\n to play again.");
        }
        else if(transientState && transientState->isGameOver)
        {
            drawGlobalRainbowMessage("Game over!\n\nPress any button\nto try again.");
        }
        else if(global.isPaused)
        {
            drawGlobalRainbowMessage("Paused");
        }
    }

    void drawGlobalRainbowMessage(const std::string &message)
    {
        float wavePhase = global.currentTime*3.0;
        size_t rainbowPhase = global.currentTime*3.0f;
        drawCenteredRainbowString(message, wavePhase, rainbowPhase);
    }

    void postProcess()
    {
        auto transientState = global.mapTransientState;
        if(!transientState)
            return;

        float fadeFactor = 1.0f;
        fadeFactor *= std::min(transientState->timeInMap/0.5f, 1.0f);
        fadeFactor *= std::max(1.0f - transientState->timeInGameOver/0.5f, 0.0f)*0.5f + 0.5f;
        fadeFactor *= std::max(1.0f - transientState->timeInGoal/0.5f, 0.0f)*0.5f + 0.5f;
        if(global.isPaused)
            fadeFactor *= 0.7f;

        if(fadeFactor >= 1.0f)
            return;

        uint32_t fadeChannelMask = std::min(uint32_t(0x7f*fadeFactor), 0xffu);


        uint32_t bitMask = fadeChannelMask | (fadeChannelMask<<8) | (fadeChannelMask << 16) | (fadeChannelMask<<24);

        auto destRow = framebuffer.pixels;
        for(uint32_t y = 0; y < framebuffer.height; ++y)
        {
            auto dest = reinterpret_cast<uint32_t*> (destRow);
            for(uint32_t x = 0; x < framebuffer.width; ++x)
            {
                *dest++ &= bitMask;
            }

            destRow += framebuffer.pitch;
        }
    }

    void render()
    {
        renderBackground();
        renderCurrentMap();
        renderHUD();
        renderActiveMessage();
        postProcess();
        renderGameStateMessage();
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
    if(canUseAPistol())
    {
        auto weaponDirection = (self->halfExtent + 0.15f)*self->lookDirection.normalized();
        auto spriteOffset = global.itemsSprites.tileExtent.asVector2F()*0.5f;
        auto weaponDisplayPosition = (renderer.worldToViewPixels(self->position+ self->spriteOffset + weaponDirection) - spriteOffset).asVector2I();

        if(self->lookDirection.y != 0)
            renderer.blitTile(global.itemsSprites, Vector2I(2, 0), weaponDisplayPosition, false, self->lookDirection.y < 0);
        else
            renderer.blitTile(global.itemsSprites, Vector2I(1, 0), weaponDisplayPosition, self->lookDirection.x < 0);
    }

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

    //renderer.fillWorldRectangle(self->debugSensor, 0xff202020);
}

void EntityInvisibleSensorBehavior::renderWith(Entity *self, Renderer &renderer)
{
    (void)self;
    (void)renderer;
    //renderer.fillWorldRectangle(self->boundingBox(), 0xff208080);
}
