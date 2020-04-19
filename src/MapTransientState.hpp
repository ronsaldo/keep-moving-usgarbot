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

enum class MapLayerType : uint8_t {
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

    // Some special layers.
    MapEntityLayerState *projectileEntityLayer;

    // The zombie entities list.
    FixedVector<Entity*, MaxNumberOfEntities> zombieEntities;

    // The free entities list.
    FixedVector<Entity*, MaxNumberOfEntities> freeEntities;

    // The entities that may affect collisions.
    FixedVector<Entity*, MaxNumberOfEntities> collisionEntities;

    // The entities that need periodical updating.
    FixedVector<Entity*, MaxNumberOfEntities> tickingEntitites;

    // Some special entities.
    Entity *activePlayer;
    Entity *activeVIP;
    bool isVipFollowingPlayer;

    // An active message
    SmallFixedString<32> currentMessage;
    float currentMessageRemainingTime;

    // Is this a game over?
    bool isGameOver;
    float timeInGameOver;
};

#endif //MAP_TRANSIENT_STATE_HPP
