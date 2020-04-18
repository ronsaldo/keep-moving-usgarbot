#include "Entity.hpp"
#include "GameLogic.hpp"
#include "MapTransientState.hpp"

#define ENTITY_BEHAVIOR_TYPE(typeName) static Entity ## typeName ## Behavior Entity ## typeName ## Behavior_singleton;
#include "EntityBehaviorTypes.inc"
#undef ENTITY_BEHAVIOR_TYPE

static EntityBehavior *entityBehaviorTypeIntoClassTable[] = {
#define ENTITY_BEHAVIOR_TYPE(typeName) &Entity ## typeName ## Behavior_singleton,
#include "EntityBehaviorTypes.inc"
#undef ENTITY_BEHAVIOR_TYPE
};

EntityBehavior *entityBehaviorTypeIntoClass(EntityBehaviorType type)
{
    return entityBehaviorTypeIntoClassTable[type];
}

void EntityCharacterBehavior::update(Entity *self, float delta)
{
    self->position += self->velocity*delta;
}

void EntityPlayerBehavior::update(Entity *self, float delta)
{
    self->velocity = Vector2F(global.controllerState.leftXAxis, global.controllerState.leftYAxis)*10.0f;

    EntityCharacterBehavior::update(self, delta);

    global.cameraPosition = self->position;
}
