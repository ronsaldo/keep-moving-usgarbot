#ifndef SIMPLE_GAME_TEMPLATE_GAME_LOGIC_INTERFACE_HPP
#define SIMPLE_GAME_TEMPLATE_GAME_LOGIC_INTERFACE_HPP

#include "GameInterface.hpp"
#include "ControllerState.hpp"
#include "Image.hpp"
#include "SoundSample.hpp"
#include "TileSet.hpp"
#include "MapFile.hpp"
#include <algorithm>

struct MapTransientState;

enum class LevelID
{
    Test = 0,
    DonMeowth,
    MrPresident,

    InitialLevel = Test,
    FinalLevel = Test,
};

struct GlobalState
{
    // Global states
    bool isInitialized;
    bool isPaused;
    bool isGameFinished;
    LevelID currentLevelID;
    float currentTime;
    ControllerState oldControllerState;
    ControllerState controllerState;

    // Assets.
    TileSet mainTileSet;
    TileSet hudTiles;
    TileSet itemsSprites;
    TileSet robotSprites;
    TileSet catDogsSprites;
    MapFilePtr currentMap;
    SoundSamplePtr noiseSample;

    // Camera/player.
    Vector2F cameraPosition;

    // The transient state. To keep the per map specific data.
    MapTransientState *mapTransientState;

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
    auto buffer = allocateTransientBytes(sizeof(T));
    new (buffer) T;
    return reinterpret_cast<T*> (buffer);
}

#endif //SIMPLE_GAME_TEMPLATE_GAME_LOGIC_INTERFACE_HPP
