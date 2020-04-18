#include "GameInterface.hpp"
#include "HostInterface.hpp"
#include "GameLogic.hpp"
#include <algorithm>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>

GlobalState *globalState;
HostInterface *hostInterface;
static MemoryZone *transientMemoryZone;

float sawToothWave(float x)
{
    return (x - floor(x))*2.0f - 1.0f;
}

float sineWave(float x)
{
    return sin(x*M_PI*2.0f);
}

uint8_t *allocateTransientBytes(size_t byteCount)
{
    return transientMemoryZone->allocateBytes(byteCount);
}

static void initializeGlobalState()
{
    if(global.isInitialized)
        return;

    // This is the place for loading the required game assets.
    global.mainTileSet.loadFrom("tileset.png");

    global.isInitialized = true;
}

void update(float delta, const ControllerState &controllerState)
{
    initializeGlobalState();
    global.oldControllerState = global.controllerState;
    global.controllerState = controllerState;

    global.currentTime += delta;

    // Pause button
    if(global.isButtonPressed(ControllerButton::Start))
        global.isPaused = !global.isPaused;
}

class Renderer
{
public:
    Renderer(const Framebuffer &f)
        : framebuffer(f) {}

    const Framebuffer &framebuffer;

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

    void render()
    {
        renderBackground();

        blitTile(global.mainTileSet, 0, 0, sawToothWave(global.currentTime*0.5)*200, -sawToothWave(global.currentTime*0.125 + 0.5)*480*0, true, false);
        /*blitTile(global.mainTileSet, 0, 0, 0, 0, false ,false);
        blitTile(global.mainTileSet, 0, 0, 32, 0, true, false);
        blitTile(global.mainTileSet, 0, 0, 0, 32, false ,true);
        blitTile(global.mainTileSet, 0, 0, 32, 32, true, true);*/
    }

    void blitTile(const TileSet &tileSet, int tileX, int tileY, int32_t x, int32_t y, bool flipX = false, bool flipY = false)
    {
        blitImage(tileSet.image, tileX*tileSet.tileWidth, tileY*tileSet.tileHeight, tileSet.tileWidth, tileSet.tileHeight, x, y, flipX, flipY);
    }

    void blitImage(const ImagePtr &image, int32_t sx, int32_t sy, int32_t w, int32_t h, int32_t x, int32_t y, bool flipX = false, bool flipY = false)
    {
        //printf("%d %d %d %d -> %d %d\n", sx, sy, w, h, x, y);

        auto destMinX = std::max(x, 0);
        auto destMaxX = std::min(x + w, (int32_t)framebuffer.width);
        if(destMinX >= destMaxX) return;

        auto destMinY = std::max(y, 0);
        auto destMaxY = std::min(y + h, (int32_t)framebuffer.height);
        if(destMinY >= destMaxY) return;

        auto copyOffsetX = destMinX - x;
        auto copyOffsetY = destMinY - y;

        auto copyX = sx + copyOffsetX;
        auto copyY = sy + copyOffsetY;

        auto sourceMinX = std::max(0, copyX);
        auto sourceMinY = std::max(0, copyY);

        auto destRow = framebuffer.pixels + framebuffer.pitch*destMinY + destMinX*4;
        auto sourceRow = image->data.get();
        if(flipX)
            sourceRow += (w - sourceMinX - 1)*4;
        else
            sourceRow += sourceMinX*4;

        if(flipY)
            sourceRow += image->pitch*(h - sourceMinY - 1);
        else
            sourceRow += image->pitch*sourceMinY;

        auto sourceRowIncrement = flipX ? -1 : 1;
        auto sourcePitch = flipY ? int(-image->pitch) : int(image->pitch);

        for(int32_t y = destMinY; y < destMaxY; ++y)
        {
            auto dest = reinterpret_cast<uint32_t*> (destRow);
            auto source = reinterpret_cast<uint32_t*> (sourceRow);
            for(int32_t x = destMinX; x < destMaxX; ++x)
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
};

void render(const Framebuffer &framebuffer)
{
    if(!global.isInitialized)
        return;

    Renderer r(framebuffer);
    r.render();
}

class GameInterfaceImpl : public GameInterface
{
public:
    virtual void setPersistentMemory(MemoryZone *zone) override;
    virtual void setTransientMemory(MemoryZone *zone) override;
    virtual void update(float delta, const ControllerState &controllerState) override;
    virtual void render(const Framebuffer &framebuffer) override;
    virtual void setHostInterface(HostInterface *theHost) override;

};

void GameInterfaceImpl::setPersistentMemory(MemoryZone *zone)
{
    globalState = reinterpret_cast<GlobalState*> (zone->getData());
}

void GameInterfaceImpl::setHostInterface(HostInterface *theHost)
{
    hostInterface = theHost;
}

void GameInterfaceImpl::setTransientMemory(MemoryZone *zone)
{
    transientMemoryZone = zone;
}

void GameInterfaceImpl::update(float delta, const ControllerState &controllerState)
{
    ::update(delta, controllerState);
}

void GameInterfaceImpl::render(const Framebuffer &framebuffer)
{
    ::render(framebuffer);
}

static GameInterfaceImpl gameInterfaceImpl;

extern "C" GameInterface *getGameInterface()
{
    return &gameInterfaceImpl;
}
