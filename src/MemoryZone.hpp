#ifndef SIMPLE_GAME_TEMPLATE_GAME_MEMORY_ZONE_HPP
#define SIMPLE_GAME_TEMPLATE_GAME_MEMORY_ZONE_HPP

#include <stddef.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

class MemoryZone
{
public:
    MemoryZone()
        : data(nullptr), size(0), currentPosition(0) {}
    ~MemoryZone()
    {
        delete [] data;
    }

    void reserve(size_t newSize)
    {
        delete [] data;
        data = new uint8_t[newSize];
        size = newSize;
        reset();
    }

    void reset()
    {
        memset(data, 0, size);
    }

    uint8_t *allocateBytes(size_t byteCount)
    {
        assert(currentPosition + byteCount <= size);
        auto result = data + currentPosition;
        currentPosition += byteCount;
        return result;
    }

    uint8_t *getData() const
    {
        return data;
    }

    void clearAll()
    {
        currentPosition = 0;
    }

private:
    uint8_t *data;
    size_t size;
    size_t currentPosition;
};

#endif //SIMPLE_GAME_TEMPLATE_GAME_MEMORY_ZONE_HPP
