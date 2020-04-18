#ifndef HOST_INTERFACE_HPP
#define HOST_INTERFACE_HPP

#include "Image.hpp"
#include "SoundSample.hpp"

struct HostInterface
{
    virtual Image *loadImage(const char *fileName) = 0;
    virtual SoundSamplePtr loadSoundSample(const char *fileName) = 0;
};

#endif //SIMPLE_GAME_TEMPLATE_GAME_INTERFACE_HPP
