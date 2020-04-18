#include "GameInterface.hpp"
#include "HostInterface.hpp"
#include "GameLogic.hpp"
#include "MapTransientState.hpp"
#include <algorithm>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <unordered_map>

GlobalState *globalState;
HostInterface *hostInterface;
static MemoryZone *transientMemoryZone;
static std::unordered_map<std::string, EntityBehaviorType> EntityTypeToBehaviorType = {
#   define ENTITY_BEHAVIOR_TYPE(typeName) {#typeName, EntityBehaviorType::typeName},
#   include "EntityBehaviorTypes.inc"
#   undef ENTITY_BEHAVIOR_TYPE
};

float sawToothWave(float x)
{
    return (x - floor(x))*2.0f - 1.0f;
}

float sineWave(float x)
{
    return sin(x*M_PI*2.0f);
}

uint8_t *allocateTransientBytes(size_t byteCount)
{
    return transientMemoryZone->allocateBytes(byteCount);
}

static Entity *instatiateEntityInLayer(MapEntityLayerState *entityLayer)
{
    global.mapTransientState->entities.push_back(Entity());
    auto result = &global.mapTransientState->entities.back();
    entityLayer->entities.push_back(result);
    return result;
}

static EntityBehaviorType parseEntityTypeName(const std::string &name)
{
    auto it = EntityTypeToBehaviorType.find(name);
    if(it != EntityTypeToBehaviorType.end())
        return it->second;
    return EntityBehaviorType::Null;
}

static Entity *instantiateMapEntity(MapEntityLayerState *entityLayer, const MapFileEntity &mapEntity)
{
    auto result = instatiateEntityInLayer(entityLayer);
    result->name = mapEntity.name;
    result->type = parseEntityTypeName(mapEntity.type);

    auto bbox = mapEntity.boundingBox;
    result->position = (bbox.center() - Vector2F(0.0f, global.currentMap->height()))*Vector2F(UnitsPerPixel, -UnitsPerPixel);
    result->halfExtent = bbox.halfExtent()*UnitsPerPixel;

    result->spawn();
    if(result->needsTicking())
        global.mapTransientState->tickingEntitites.push_back(result);

    return result;
}

static void loadMapFileEntityLayer(const MapFileEntityLayer &layer)
{
    auto entityLayer = newTransient<MapEntityLayerState> ();
    global.mapTransientState->layers.push_back(entityLayer);

    for(uint32_t i = 0; i < layer.entityCount; ++i)
    {
        instantiateMapEntity(entityLayer, layer.entities[i]);
    }

}

static void loadMapFileTileLayer(const MapFileTileLayer &layer)
{
    auto solidLayer = newTransient<MapSolidLayerState> ();
    solidLayer->mapTileLayer = &layer;

    global.mapTransientState->layers.push_back(solidLayer);
}

static void loadMapFile(const char *filename)
{
    // Do the actual map loading.
    global.currentMap.reset(hostInterface->loadMapFile(filename));

    transientMemoryZone->reset();
    transientMemoryZone->clearAll();
    global.mapTransientState = newTransient<MapTransientState> ();
    global.cameraPosition = Vector2F::zeros();

    // Clear the map states.
    global.currentMap->layersDo([&](const MapFileLayer &layer) {
        switch(layer.type)
        {
        case MapFileLayerType::Entities:
            loadMapFileEntityLayer(reinterpret_cast<const MapFileEntityLayer &> (layer));
            break;
        case MapFileLayerType::Tiles:
            loadMapFileTileLayer(reinterpret_cast<const MapFileTileLayer &> (layer));
            break;
        default:
            // Ignored
            break;
        }
    });
}

static void initializeGlobalState()
{
    if(global.isInitialized)
        return;

    // This is the place for loading the required game assets.
    global.mainTileSet.loadFrom("tileset.png");
    loadMapFile("test.map");

    global.isInitialized = true;
}

static void updateTransientState(float delta)
{
    if(!global.mapTransientState)
        return;

    // Update the ticking entities.
    for(auto entity : global.mapTransientState->tickingEntitites)
        entity->update(delta);
}

void update(float delta, const ControllerState &controllerState)
{
    initializeGlobalState();
    global.oldControllerState = global.controllerState;
    global.controllerState = controllerState;

    updateTransientState(delta);

    global.currentTime += delta;

    // Pause button
    if(global.isButtonPressed(ControllerButton::Start))
        global.isPaused = !global.isPaused;
}

void render(const Framebuffer &framebuffer);

class GameInterfaceImpl : public GameInterface
{
public:
    virtual void setPersistentMemory(MemoryZone *zone) override;
    virtual void setTransientMemory(MemoryZone *zone) override;
    virtual void update(float delta, const ControllerState &controllerState) override;
    virtual void render(const Framebuffer &framebuffer) override;
    virtual void setHostInterface(HostInterface *theHost) override;

};

void GameInterfaceImpl::setPersistentMemory(MemoryZone *zone)
{
    globalState = reinterpret_cast<GlobalState*> (zone->getData());
}

void GameInterfaceImpl::setHostInterface(HostInterface *theHost)
{
    hostInterface = theHost;
}

void GameInterfaceImpl::setTransientMemory(MemoryZone *zone)
{
    transientMemoryZone = zone;
}

void GameInterfaceImpl::update(float delta, const ControllerState &controllerState)
{
    ::update(delta, controllerState);
}

void GameInterfaceImpl::render(const Framebuffer &framebuffer)
{
    ::render(framebuffer);
}

static GameInterfaceImpl gameInterfaceImpl;

extern "C" GameInterface *getGameInterface()
{
    return &gameInterfaceImpl;
}
