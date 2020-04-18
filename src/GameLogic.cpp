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
    global.currentMap.reset(hostInterface->loadMapFile("test.map"));

    global.isInitialized = true;
}

void update(float delta, const ControllerState &controllerState)
{
    initializeGlobalState();
    global.oldControllerState = global.controllerState;
    global.controllerState = controllerState;

    global.cameraPosition += Vector2F(global.controllerState.leftXAxis, global.controllerState.leftYAxis)*delta*10.0;

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
    Vector2I cameraPixelOffset;

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
        cameraPixelOffset = (-Vector2F(global.cameraPosition.x, -global.cameraPosition.y)*PixelsPerUnit).floor().asVector2I();
        renderBackground();

        global.currentMap->tileLayersDo([&](const MapFileTileLayer &layer) {
            renderTileLayer(layer);
        });
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
