#include "Entity.hpp"
#include "GameLogic.hpp"
#include "MapTransientState.hpp"
#include "Collisions.hpp"

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


void Entity::spawn()
{
    auto selfClass = entityBehaviorTypeIntoClass(type);
    selfClass->spawn(this);
    if(selfClass->needsTicking(this))
        global.mapTransientState->tickingEntitites.push_back(this);

}

void EntityBulletBehavior::spawn(Entity *self)
{
    self->color = 0xff80cccc;
}

void EntityBulletBehavior::update(Entity *self, float delta)
{
    self->position += self->velocity*delta;

    self->remainingLifeTime -= delta;
    if(self->remainingLifeTime < 0)
        self->kill();
}

void EntityKinematicCollidingBehavior::sweepCollidingAlongSegment(Entity *self, const Vector2F &segmentStartPoint, const Vector2F &segmentEndPoint, uint32_t maxDepth)
{
    if(maxDepth == 0)
        return;

    auto collisionRay = Ray2F::fromSegment(segmentStartPoint, segmentEndPoint);
    CollisionSweepTestResult collisionTestResult;

    // No collision, nothing interesting is required.
    sweepCollisionBoxAlongRay(self->halfExtent, collisionRay, collisionTestResult);
    if(!collisionTestResult.hasCollision)
    {
        self->position = segmentEndPoint;
        return;
    }

    auto collisionPoint = collisionRay.pointAtT(collisionTestResult.collisionDistance);
    auto collisionNormalAndPenetration = collisionTestResult.collidingBox.computeCollisionNormalAndPenetrationAtPoint(collisionPoint);
    auto collisionNormal = collisionNormalAndPenetration.first;
    auto penetrationDistance = collisionNormalAndPenetration.second;

    // Supress the velocity along the collision normal.
    auto tangentMask = Vector2F::ones() - collisionNormal.abs();
    self->velocity *= tangentMask;

    // Resolve the collision.
    self->position = collisionPoint + collisionNormal*(penetrationDistance + CollisionNotStuckEpsilon);

    // Try to keep sliding along the tangent.
    auto remainingT = collisionRay.maxT - collisionTestResult.collisionDistance;
    if(remainingT < CollisionSweepStopEpsilon)
        return;

    auto tangentDirection = collisionRay.direction*tangentMask;
    auto nextSegmentEnd = self->position + tangentDirection*remainingT;
    sweepCollidingAlongSegment(self, self->position, nextSegmentEnd, maxDepth - 1);
}

void EntityKinematicCollidingBehavior::update(Entity *self, float delta)
{
    // Euler method.
    auto acceleration = self->acceleration - self->damping*self->velocity;
    self->velocity += acceleration*delta;
    auto newPosition = self->position + self->velocity*delta;

    sweepCollidingAlongSegment(self, self->position, newPosition);
}

void EntityCharacterBehavior::spawn(Entity *self)
{
    Super::spawn(self);
    self->lookDirection = Vector2F(1.0f, 0.0f);
    self->acceleration = myGravity();
    self->computeDampingForTerminalVelocity(myTerminalVelocity() + fallTerminalVelocity(), myEngineAcceleration() + myGravity());

}

void EntityCharacterBehavior::update(Entity *self, float delta)
{
    //if(isOnFloor(self))
    self->acceleration = myGravity() + self->walkDirection*myEngineAcceleration();

    Super::update(self, delta);
}

bool EntityCharacterBehavior::isOnFloor(Entity *self)
{
    auto floorSensor = Box2F::withCenterAndHalfExtent(self->position, Vector2F(self->halfExtent.x, 0.1f))
        .translatedBy(Vector2F(0.0f, -self->halfExtent.y - 0.1f));
    return isBoxCollidingWithSolid(floorSensor);
}

bool EntityCharacterBehavior::canJump(Entity *self)
{
    return isOnFloor(self);
}

void EntityCharacterBehavior::jump(Entity *self)
{
    if(canJump(self))
        self->velocity += jumpVelocity();
}

void EntityCharacterBehavior::shoot(Entity *self, float bulletSpeed, float bulletLifeTime, int currentAmmunitionPower)
{
    auto bullet = instatiateEntityInLayer(global.mapTransientState->projectileEntityLayer, EntityBehaviorType::Bullet);
    bullet->position = self->position;
    bullet->velocity = self->velocity + self->lookDirection*bulletSpeed;
    bullet->halfExtent = 0.2f;
    bullet->owner = self;
    bullet->remainingLifeTime = bulletLifeTime;
    bullet->contactDamage = currentAmmunitionPower;
    bullet->spawn();
}

void EntityCharacterBehavior::shoot(Entity *self)
{
    shoot(self, currentAmmunitionSpeed(self), currentAmmunitionDuration(self), currentAmmunitionPower(self));
}

void EntityPlayerBehavior::spawn(Entity *self)
{
    Super::spawn(self);

    self->setExtent(Vector2F(0.7f, 1.7f));
}

void EntityPlayerBehavior::update(Entity *self, float delta)
{
    self->updateLookDirectionWithNormalizedVector(Vector2F(global.controllerState.leftXAxis, global.controllerState.leftYAxis));

    self->walkDirection = Vector2F(global.controllerState.leftXAxis, 0.0f);
    if(global.isButtonPressed(ControllerButton::A))
        jump(self);
    if(global.isButtonPressed(ControllerButton::X))
        shoot(self);

    Super::update(self, delta);

    global.cameraPosition = self->position;
}

void EntityEnemyBehavior::spawn(Entity *self)
{
    Super::spawn(self);
    self->setExtent(Vector2F(0.7f, 1.7f));
    self->color = 0xffcccc00;
}
