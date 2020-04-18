#include "GameInterface.hpp"
#include "HostInterface.hpp"
#include "GameLogic.hpp"
#include <algorithm>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

GlobalState *globalState;
HostInterface *hostInterface;
static MemoryZone *transientMemoryZone;

uint8_t *allocateTransientBytes(size_t byteCount)
{
    return transientMemoryZone->allocateBytes(byteCount);
}

static void initializeGlobalState()
{
    if(global.isInitialized)
        return;

    // TODO. This is the place for loading the required game assets..
    global.noiseSample = hostInterface->loadSoundSample("noise.wav");
    global.noiseSample->play(true);

    global.isInitialized = true;
}

void update(float delta, const ControllerState &controllerState)
{
    initializeGlobalState();
    global.oldControllerState = global.controllerState;
    global.controllerState = controllerState;

    // TODO: Perform time dependant updates by using the delta.
    (void)delta;

    // Pause button
    if(global.isButtonPressed(ControllerButton::Start))
        global.isPaused = !global.isPaused;
}

void render(const Framebuffer &framebuffer)
{
    auto destRow = framebuffer.pixels;
    for(uint32_t y = 0; y < framebuffer.height; ++y)
    {
        auto dest = reinterpret_cast<uint32_t*> (destRow);
        for(uint32_t x = 0; x < framebuffer.width; ++x)
            dest[x] = (x & 0xff) | ((y & 0xFF) << 8) | 0xff000000;

        destRow += framebuffer.pitch;
    }
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
