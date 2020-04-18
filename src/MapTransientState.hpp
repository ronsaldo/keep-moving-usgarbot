#ifndef MAP_TRANSIENT_STATE_HPP
#define MAP_TRANSIENT_STATE_HPP

#include "FixedVector.hpp"
#include "MapFile.hpp"
#include "Entity.hpp"

enum {
    MaxNumberOfLayers = 5,
    MaxNumberOfEntities = 4096,
    MaxNumberOfEntitiesPerLayer = 512,
};

enum MapLayerType : uint8_t {
    Solid,
    Entities
};

struct MapLayerStateCommon
{
    MapLayerType type;
};

struct MapSolidLayerState : public MapLayerStateCommon
{
    MapSolidLayerState()
    {
        type = MapLayerType::Solid;
    }

    const MapFileTileLayer *mapTileLayer;
};

struct MapEntityLayerState : public MapLayerStateCommon
{
    MapEntityLayerState()
    {
        type = MapLayerType::Entities;
    }

    FixedVector<Entity*, MaxNumberOfEntitiesPerLayer> entities;
};

struct MapTransientState
{
    // Per-layer required state.
    FixedVector<MapLayerStateCommon*, MaxNumberOfLayers> layers;

    // The actual list of entities.
    FixedVector<Entity, MaxNumberOfEntities> entities;

    // The free entities list.
    FixedVector<Entity*, MaxNumberOfEntities> freeEntities;

    // The entities that may affect collisions.
    FixedVector<Entity*, MaxNumberOfEntities> collisionEntities;

    // The entities that need periodical updating.
    FixedVector<Entity*, MaxNumberOfEntities> tickingEntitites;

};

#endif //MAP_TRANSIENT_STATE_HPP
