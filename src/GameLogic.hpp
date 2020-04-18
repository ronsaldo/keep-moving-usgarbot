#ifndef SIMPLE_GAME_TEMPLATE_GAME_LOGIC_INTERFACE_HPP
#define SIMPLE_GAME_TEMPLATE_GAME_LOGIC_INTERFACE_HPP

#include "GameInterface.hpp"
#include "ControllerState.hpp"
#include "Image.hpp"
#include "SoundSample.hpp"
#include "TileSet.hpp"
#include "MapFile.hpp"
#include <algorithm>

#define PixelsPerUnit 32.0f

struct GlobalState
{
    // Global states
    bool isInitialized;
    bool isPaused;
    bool isGameCompleted;
    float currentTime;
    ControllerState oldControllerState;
    ControllerState controllerState;

    // Assets.
    TileSet mainTileSet;
    MapFilePtr currentMap;
    SoundSamplePtr noiseSample;

    // Camera/player.
    Vector2F cameraPosition;

    bool isButtonPressed(int button) const
    {
        return controllerState.getButton(button) && !oldControllerState.getButton(button);
    }
};

static_assert(sizeof(GlobalState) < PersistentMemorySize, "Increase the persistentMemory");

extern GlobalState *globalState;
extern HostInterface *hostInterface;

#define global (*globalState)

uint8_t *allocateTransientBytes(size_t byteCount);

template<typename T>
T *newTransient()
{
    return allocateTransientBytes(sizeof(T));
}

#endif //SIMPLE_GAME_TEMPLATE_GAME_LOGIC_INTERFACE_HPP
