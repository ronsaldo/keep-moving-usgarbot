#ifndef SIMPLE_GAME_TEMPLATE_SOUND_SAMPLE_HPP
#define SIMPLE_GAME_TEMPLATE_SOUND_SAMPLE_HPP

#include <stdint.h>

class SoundSample
{
public:
    virtual ~SoundSample() {};

    virtual void play(bool looped) = 0;
    virtual void resume() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
};

typedef SoundSample* SoundSamplePtr;

#endif //SIMPLE_GAME_TEMPLATE_IMAGE_HPP
