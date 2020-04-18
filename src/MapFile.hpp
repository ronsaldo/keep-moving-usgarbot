#ifndef MAP_FILE_HPP
#define MAP_FILE_HPP

#include <stdint.h>
#include <memory>
#include "Vector2.hpp"
#include "Box2.hpp"
#include "FixedString.hpp"

#define MapFileHeader_MagicCode "MAP "
#define MapFileHeader_MagicCodeSize 4

enum class MapFileLayerType : uint8_t {
    Tiles = 0,
    Entities = 1,
};

struct MapFileHeader
{
    uint8_t magicCode[MapFileHeader_MagicCodeSize];

    // The dimensions.
    Vector2I extent;
    Vector2I tileExtent;

    // The layers
    uint32_t layerCount;
    uint32_t layerOffsets[];

    bool hasValidMagicNumber() const
    {
        return memcmp(magicCode, MapFileHeader_MagicCode, MapFileHeader_MagicCodeSize) == 0;
    }
};

struct MapFileLayer
{
    MapFileLayerType type;
    uint8_t reserved;
    uint16_t reserved2;

    SmallFixedString<32> name;
};

struct MapFileTileLayer : public MapFileLayer
{
    Vector2I extent;
    uint16_t tiles[]; // extent x * extent y tiles.
};

struct MapFileEntity
{
    SmallFixedString<32> name;
    SmallFixedString<32> type;
    Box2F boundingBox;
};

struct MapFileEntityLayer : public MapFileLayer
{
    uint32_t entityCount;
};

class MapFile
{
public:
    bool validate() const
    {
        return header().hasValidMagicNumber();
    }

    const MapFileHeader &header() const
    {
        return *reinterpret_cast<MapFileHeader*> (mapFileContent.get());
    }

    template<typename FT>
    void layersDo(const FT &f) {
        auto &h = header();
        for(uint32_t i = 0; i < h.layerCount; ++i)
        {
            auto layer = reinterpret_cast<const MapFileLayer*> (mapFileContent.get() + h.layerOffsets[i]);
            f(*layer);
        }
    }

    template<typename FT>
    void tileLayersDo(const FT &f) {
        layersDo([&](const MapFileLayer &layer)
        {
            if(layer.type == MapFileLayerType::Tiles)
                f(reinterpret_cast<const MapFileTileLayer &> (layer));
        });
    }

    size_t mapFileSize;
    std::unique_ptr<uint8_t[]> mapFileContent;
};

typedef std::unique_ptr<MapFile> MapFilePtr;

#endif //MAP_FILE_HPP