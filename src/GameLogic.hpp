#ifndef SIMPLE_GAME_TEMPLATE_GAME_LOGIC_INTERFACE_HPP
#define SIMPLE_GAME_TEMPLATE_GAME_LOGIC_INTERFACE_HPP

#include "GameInterface.hpp"
#include "ControllerState.hpp"
#include "Image.hpp"
#include "SoundSample.hpp"
#include <algorithm>

struct GlobalState
{
    // Global states
    bool isInitialized;
    bool isPaused;
    bool isGameCompleted;
    float currentTime;
    float matchTime;
    ControllerState oldControllerState;
    ControllerState controllerState;

    // Assets.
    SoundSamplePtr noiseSample;

    bool isButtonPressed(int button) const
    {
        return controllerState.getButton(button) && !oldControllerState.getButton(button);
    }
};

static_assert(sizeof(GlobalState) < PersistentMemorySize, "Increase the persistentMemory");

extern GlobalState *globalState;

#define global (*globalState)

uint8_t *allocateTransientBytes(size_t byteCount);

template<typename T>
T *newTransient()
{
    return allocateTransientBytes(sizeof(T));
}

#endif //SIMPLE_GAME_TEMPLATE_GAME_LOGIC_INTERFACE_HPP
