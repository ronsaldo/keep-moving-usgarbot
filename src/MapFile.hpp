#ifndef MAP_FILE_HPP
#define MAP_FILE_HPP

#include <stdint.h>
#include <memory>
#include "Vector2.hpp"
#include "Box2.hpp"
#include "FixedString.hpp"
#include "Coordinates.hpp"

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

    Box2I tileGridBounds() const
    {
        return Box2I(0.0f, extent);
    }

    Vector2F pointFromPixelIntoTileSpace(const Vector2F &point, const Vector2F tileExtent) const
    {
        return point/(Vector2F(tileExtent.x, tileExtent.y)) + Vector2F(0.0f, extent.y);
    }

    Vector2F vectorFromPixelIntoTileSpace(const Vector2F &vector, const Vector2F tileExtent) const
    {
        return vector / tileExtent;
    }

    inline Box2F boxFromPixelIntoTileSpace(const Box2F &box, const Vector2F tileExtent) const
    {
        return Box2F::withCenterAndHalfExtent(pointFromPixelIntoTileSpace(box.center(), tileExtent), vectorFromPixelIntoTileSpace(box.halfExtent(), tileExtent));
    }

    Vector2F pointFromWorldIntoTileSpace(const Vector2F &point, const Vector2F tileExtent) const
    {
        return pointFromPixelIntoTileSpace(pointFromWorldIntoPixelSpace(point), tileExtent);
    }

    Vector2F vectorFromWorldIntoTileSpace(const Vector2F &vector, const Vector2F tileExtent) const
    {
        return vectorFromPixelIntoTileSpace(vectorFromWorldIntoPixelSpace(vector), tileExtent);
    }

    inline Box2F boxFromWorldIntoTileSpace(const Box2F &box, const Vector2F tileExtent) const
    {
        return Box2F::withCenterAndHalfExtent(pointFromWorldIntoTileSpace(box.center(), tileExtent), vectorFromWorldIntoTileSpace(box.halfExtent(), tileExtent));
    }

    inline Ray2F rayFromWorldIntoTileSpace(const Ray2F &ray, const Vector2F tileExtent) const
    {
        return Ray2F::fromSegment(pointFromWorldIntoTileSpace(ray.startPoint(), tileExtent), pointFromWorldIntoTileSpace(ray.endPoint(), tileExtent));
    }

    Vector2F pointFromTileIntoPixelSpace(const Vector2F &point, const Vector2F tileExtent) const
    {
        return (point - Vector2F(0.0f, extent.y))*(Vector2F(tileExtent.x, tileExtent.y));
    }

    Vector2F vectorFromTileIntoPixelSpace(const Vector2F &vector, const Vector2F tileExtent) const
    {
        return vector * tileExtent;
    }

    inline Box2F boxFromTileIntoPixelSpace(const Box2F &box, const Vector2F tileExtent) const
    {
        return Box2F::withCenterAndHalfExtent(pointFromTileIntoPixelSpace(box.center(), tileExtent), vectorFromTileIntoPixelSpace(box.halfExtent(), tileExtent));
    }

    Vector2F pointFromTileIntoWorldSpace(const Vector2F &point, const Vector2F tileExtent) const
    {
        return pointFromPixelIntoWorldSpace(pointFromTileIntoPixelSpace(point, tileExtent));
    }

    Vector2F vectorFromTileIntoWorldSpace(const Vector2F &vector, const Vector2F tileExtent) const
    {
        return vectorFromPixelIntoWorldSpace(vectorFromTileIntoPixelSpace(vector, tileExtent));
    }

    inline Box2F boxFromTileIntoWorldSpace(const Box2F &box, const Vector2F tileExtent) const
    {
        return Box2F::withCenterAndHalfExtent(pointFromTileIntoWorldSpace(box.center(), tileExtent), vectorFromTileIntoWorldSpace(box.halfExtent(), tileExtent));
    }

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
    MapFileEntity entities[];
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

    int32_t height() const
    {
        return header().extent.y * header().tileExtent.y;
    }

    Vector2I extent() const
    {
        return header().extent * header().tileExtent;
    }

    Vector2I tileExtent() const
    {
        return header().tileExtent;
    }

    size_t mapFileSize;
    std::unique_ptr<uint8_t[]> mapFileContent;
};

typedef std::unique_ptr<MapFile> MapFilePtr;

#endif //MAP_FILE_HPP
