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
    return entityBehaviorTypeIntoClassTable[(int)type];
}

void Entity::spawn()
{
    auto selfClass = entityBehaviorTypeIntoClass(type);
    selfClass->spawn(this);
    if(selfClass->needsTicking(this))
        global.mapTransientState->tickingEntitites.push_back(this);
    if(selfClass->hasCollisions(this))
        global.mapTransientState->collisionEntities.push_back(this);
}

//============================================================================
// EntityBulletBehavior
//============================================================================

void EntityBulletBehavior::spawn(Entity *self)
{
    self->color = 0xff80cccc;
}

void EntityBulletBehavior::update(Entity *self, float delta)
{
    auto oldPosition = self->position;
    auto newPosition = self->position + self->velocity*delta;

    auto collisionRay = Ray2F::fromSegment(oldPosition, newPosition);
    CollisionSweepTestResult collisionTestResult;
    collisionTestResult.entityExclusionSet.push_back(self);
    if(self->owner)
        collisionTestResult.entityExclusionSet.push_back(self->owner);

    // No collision, nothing interesting is required.
    sweepCollisionBoxAlongRay(self->halfExtent, collisionRay, collisionTestResult);
    if(collisionTestResult.hasCollision)
    {
        if(collisionTestResult.collidingEntity)
        {
            collisionTestResult.collidingEntity->hurtAt(self->contactDamage,
                collisionRay.pointAtT(collisionTestResult.collisionDistance),
                self->velocity*self->mass);
        }

        // We hit something. For now I am done.
        self->kill();
        return;
    }

    self->position = newPosition;
    self->remainingLifeTime -= delta;
    if(self->remainingLifeTime < 0)
        self->kill();
}

//============================================================================
// EntityKinematicCollidingBehavior
//============================================================================

void EntityKinematicCollidingBehavior::sweepCollidingAlongSegment(Entity *self, const Vector2F &segmentStartPoint, const Vector2F &segmentEndPoint, uint32_t maxDepth)
{
    if(maxDepth == 0)
        return;

    auto collisionRay = Ray2F::fromSegment(segmentStartPoint, segmentEndPoint);
    CollisionSweepTestResult collisionTestResult;
    collisionTestResult.entityExclusionSet.push_back(self);

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

    if(collisionTestResult.collidingEntity)
    {
        collisionTestResult.collidingEntity->applyImpulse(self->velocity*self->mass*collisionNormal.abs()*self->coefficientOfRestitution);
    }

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

//============================================================================
// EntityCharacterBehavior
//============================================================================

void EntityCharacterBehavior::spawn(Entity *self)
{
    Super::spawn(self);
    self->setMass(65.0f);
    self->coefficientOfRestitution = 0.8f;
    self->hitPoints = 1;
    self->lookDirection = Vector2F(1.0f, 0.0f);
    self->acceleration = myGravity();
    self->computeDampingForTerminalVelocity(myTerminalVelocity() + fallTerminalVelocity(), myEngineAcceleration() + myGravity());
}

void EntityCharacterBehavior::die(Entity *self)
{
    self->kill();
}

void EntityCharacterBehavior::update(Entity *self, float delta)
{
    if(self->hitPoints == 0)
    {
        die(self);
        return;
    }

    if(self->remainingInvincibilityTime > 0.0f)
        self->remainingInvincibilityTime -= delta;

    if(self->remainingBulletReloadTime > 0.0f)
        self->remainingBulletReloadTime -= delta;

    if(self->remainingActionTime > 0.0f)
        self->remainingActionTime -= delta;


    self->acceleration = myGravity() + self->walkDirection*myEngineAcceleration();

    Super::update(self, delta);
}

bool EntityCharacterBehavior::isOnFloor(Entity *self)
{
    auto floorSensor = Box2F::withCenterAndHalfExtent(self->position, Vector2F(self->halfExtent.x, 0.1f))
        .translatedBy(Vector2F(0.0f, -self->halfExtent.y - 0.1f));
    //self->debugSensor = floorSensor;
    return isBoxCollidingWithSolid(floorSensor, {self});
}

bool EntityCharacterBehavior::hasFloorForward(Entity *self)
{
    auto testGap = forwardTestGap(self);

    auto wallSensorSign = self->lookDirection.x >= 0 ? 1.0f : -1.0f;
    auto wallSensorPosition = Vector2F((self->halfExtent.x + testGap + 0.1f)*wallSensorSign, - 0.1f);
    auto wallSensor = Box2F::withCenterAndHalfExtent(self->position, Vector2F(0.1f, self->halfExtent.y + 0.05f))
        .translatedBy(wallSensorPosition);

    //self->debugSensor = wallSensor;
    return isBoxCollidingWithSolid(wallSensor, {self});;
}

bool EntityCharacterBehavior::hasWallForward(Entity *self)
{
    auto testGap = forwardTestGap(self);
    auto wallSensorSign = self->lookDirection.x >= 0 ? 1.0f : -1.0f;
    auto wallSensorPosition = Vector2F((self->halfExtent.x + testGap + 0.1f)*wallSensorSign, 0.0f);
    auto wallSensor = Box2F::withCenterAndHalfExtent(self->position, Vector2F(0.1f, self->halfExtent.y*0.9f))
        .translatedBy(wallSensorPosition);

    //self->debugSensor = wallSensor;
    return isBoxCollidingWithSolid(wallSensor, {self});
}

bool EntityCharacterBehavior::canJump(Entity *self)
{
    return isOnFloor(self);
}

void EntityCharacterBehavior::jump(Entity *self)
{
    if(!canJump(self))
        return;

    self->velocity += jumpVelocity();
}

bool EntityCharacterBehavior::canDash(Entity *self)
{
    return isOnFloor(self);
}

void EntityCharacterBehavior::dash(Entity *self)
{
    if(!canDash(self))
        return;

    self->velocity += dashSpeed() * Vector2F(self->lookDirection.x, 0.0f);
}

void EntityCharacterBehavior::shoot(Entity *self, float bulletSpeed, float bulletLifeTime, int currentAmmunitionPower, float bulletMass)
{
    if(self->remainingBulletReloadTime > 0.0f)
        return;

    self->remainingBulletReloadTime = currentBulletReloadTime(self);

    auto bullet = instatiateEntityInLayer(global.mapTransientState->projectileEntityLayer, EntityBehaviorType::Bullet);
    bullet->position = self->position;
    bullet->velocity = self->velocity + self->lookDirection*bulletSpeed;
    bullet->halfExtent = 0.2f;
    bullet->owner = self;
    bullet->remainingLifeTime = bulletLifeTime;
    bullet->contactDamage = currentAmmunitionPower;
    bullet->setMass(bulletMass);
    bullet->spawn();
}

void EntityCharacterBehavior::shoot(Entity *self)
{
    shoot(self, currentAmmunitionSpeed(self), currentAmmunitionDuration(self), currentAmmunitionPower(self), currentAmmunitionMass(self));
}

void EntityCharacterBehavior::hurtAt(Entity *self, float damage, const Vector2F &hitPoint, const Vector2F &hitImpulse)
{
    (void)hitPoint;
    if(self->isInvincible())
        return;

    if(damage == 0.0f)
        return;

    //printf("Character shot %f at %f %f vel %f %f\n", damage, hitPoint.x, hitPoint.y, hitImpulse.x, hitImpulse.y);
    self->applyImpulse(hitImpulse);
    self->hitPoints = std::max(self->hitPoints - damage, 0.0f);
    self->remainingInvincibilityTime = defaultInvincibilityPeriodDuration();
}

//============================================================================
// EntityPlayerBehavior
//============================================================================

void EntityPlayerBehavior::spawn(Entity *self)
{
    Super::spawn(self);

    global.mapTransientState->activePlayer = self;
    self->hitPoints = 100;

    //self->setExtent(Vector2F(0.7f, 1.7f));
    self->setExtent(Vector2F(1.25f, 1.8f));
    self->spriteSheet = &global.robotSprites;
    self->spriteIndex = Vector2I(0, 0);
    self->spriteOffset = Vector2F(0.0f, 0.1f + -4*UnitsPerPixel);
}

void EntityPlayerBehavior::update(Entity *self, float delta)
{
    if(global.mapTransientState->isGameOver)
    {
        Super::update(self, delta);

        global.cameraPosition = self->position;
        return;
    }

    self->updateLookDirectionWithNormalizedVector(Vector2F(global.controllerState.leftXAxis, global.controllerState.leftYAxis));
    self->updateLookDirectionWithNormalizedVector(Vector2F(global.controllerState.rightXAxis, global.controllerState.rightYAxis));

    self->walkDirection = Vector2F(global.controllerState.leftXAxis, 0.0f);
    if(self->lookDirection.y == 0)
    {
        self->spriteIndex = Vector2I(0, 0);
        self->spriteFlipX = self->lookDirection.x < 0;
    }
    else if(self->lookDirection.y > 0)
    {
        self->spriteIndex = Vector2I(1, 0);
        self->spriteFlipX = self->lookDirection.x < 0;
    }
    else if(self->lookDirection.y < 0)
    {
        self->spriteIndex = Vector2I(2, 0);
        self->spriteFlipX = self->lookDirection.x < 0;
    }

    if(global.isButtonPressed(ControllerButton::A))
        jump(self);
    if(global.isButtonPressed(ControllerButton::X) || global.isButtonPressed(ControllerButton::RightTrigger))
        shoot(self);
    if(global.isButtonPressed(ControllerButton::B))
        global.mapTransientState->isVipFollowingPlayer = !global.mapTransientState->isVipFollowingPlayer;

    if(global.isButtonPressed(ControllerButton::LeftTrigger))
        dash(self);


    Super::update(self, delta);

    global.cameraPosition = self->position;
}

//============================================================================
// EntityEnemyBehavior
//============================================================================

bool EntityEnemyBehavior::hasTargetOnSight(Entity *self, Entity *testTarget)
{
    auto testRay = Ray2F::fromSegment(self->position, testTarget->position);

    // The target cannot be much farther.
    if(testRay.maxT > maximumTargetSightDistance(self))
        return false;

    // Check whether we are looking on the correct direction.
    if(self->lookDirection.y == 0)
    {
        if(self->lookDirection.x * testRay.direction.x < 0)
            return false;
    }
    else
    {
        if(self->lookDirection.x * testRay.direction.x < 0)
            return false;
    }

    // Check whether we are looking on the correct direction.
    CollisionSweepTestResult collisionTestResult;
    collisionTestResult.entityExclusionSet.push_back(self);

    // No collision, nothing interesting is required.
    sweepCollisionBoxAlongRay(0.0f, testRay, collisionTestResult);

    return collisionTestResult.hasCollision && collisionTestResult.collidingEntity == testTarget;
}

bool EntityEnemyBehavior::hasSomeTargetOnSight(Entity *self)
{
    auto transientState = global.mapTransientState;
    if(transientState->activeVIP && hasTargetOnSight(self, transientState->activeVIP))
        return true;
    if(transientState->activePlayer && hasTargetOnSight(self, transientState->activePlayer))
        return true;
    return false;
}

void EntityEnemyBehavior::spawn(Entity *self)
{
    Super::spawn(self);
    self->setExtent(Vector2F(0.7f, 1.7f));
    self->color = 0xffcccc00;
    self->hitPoints = 20;

    self->setExtent(Vector2F(1.25f, 1.8f));
    /*self->spriteSheet = &global.robotSprites;
    self->spriteIndex = Vector2I(0, 0);
    self->spriteOffset = Vector2F(0.0f, 0.1f + -4*UnitsPerPixel);*/

}

//============================================================================
// EntityEnemySentryBehavior
//============================================================================

void EntityEnemySentryBehavior::spawn(Entity *self)
{
    Super::spawn(self);
}

void EntityEnemySentryBehavior::update(Entity *self, float delta)
{
    if(hasSomeTargetOnSight(self))
    {
        shoot(self);
        self->remainingActionTime = directionLookingTime();
    }

    if(self->remainingActionTime <= 0.0f)
    {
        self->lookDirection.x = -self->lookDirection.x;
        self->remainingActionTime = directionLookingTime();
    }
    self->spriteFlipX = self->lookDirection.x < 0.0f;

    Super::update(self, delta);
}

//============================================================================
// EntityEnemyTurretBehavior
//============================================================================

void EntityEnemyTurretBehavior::spawn(Entity *self)
{
    Super::spawn(self);
}

void EntityEnemyTurretBehavior::update(Entity *self, float delta)
{
    if(hasSomeTargetOnSight(self))
        shoot(self);

    Super::update(self, delta);
}

//============================================================================
// EntityEnemyPatrolBehavior
//============================================================================

void EntityEnemyPatrolBehavior::spawn(Entity *self)
{
    Super::spawn(self);
}

void EntityEnemyPatrolBehavior::update(Entity *self, float delta)
{
    if(hasSomeTargetOnSight(self))
    {
        shoot(self);
        self->walkDirection = 0.0f;
    }
    else
    {
        if(!hasFloorForward(self) || hasWallForward(self))
        {
            self->lookDirection.x = -self->lookDirection.x;
        }
        self->walkDirection = Vector2F(self->lookDirection.x, 0.0f);
    }

    self->spriteFlipX = self->lookDirection.x < 0.0f;

    Super::update(self, delta);
}

//============================================================================
// EntityEnemyPatrolDogBehavior
//============================================================================
void EntityEnemyPatrolDogBehavior::spawn(Entity *self)
{
    Super::spawn(self);

    self->setExtent(Vector2F(2.0f, 1.0f));
    self->spriteSheet = &global.catDogsSprites;
    self->spriteIndex = Vector2I(0, 1);
    self->spriteOffset = Vector2F(0.0f, 0.1f + -4*UnitsPerPixel)*0.0f;
}

//============================================================================
// EntityEnemySentryDogBehavior
//============================================================================
void EntityEnemySentryDogBehavior::spawn(Entity *self)
{
    Super::spawn(self);

    self->setExtent(Vector2F(2.0f, 1.0f));
    self->spriteSheet = &global.catDogsSprites;
    self->spriteIndex = Vector2I(0, 1);
    self->spriteOffset = Vector2F(0.0f, 0.1f + -4*UnitsPerPixel)*0.0f;
}

//============================================================================
// EntityScoltedVIPBehavior
//============================================================================
void EntityScoltedVIPBehavior::spawn(Entity *self)
{
    Super::spawn(self);
    global.mapTransientState->activeVIP = self;

    self->hitPoints = 100;
    self->color = 0xffcc00cc;
}

void EntityScoltedVIPBehavior::update(Entity *self, float delta)
{
    auto activePlayer = global.mapTransientState->activePlayer;
    if(activePlayer)
    {
        auto playerVipVector = activePlayer->position - self->position;
        auto playerVipDistance = playerVipVector.length();

        self->lookDirection = playerVipVector.sign();
        if(self->lookDirection.x == 0 && self->lookDirection.y == 0)
            self->lookDirection = Vector2F(1.0f, 0.0f);

        if(playerVipDistance > 5.0f && global.mapTransientState->isVipFollowingPlayer)
        {
            if(isOnFloor(self))
            {
                self->walkDirection = Vector2F(self->lookDirection.x, 0.0f);
                if(hasWallForward(self))
                    jump(self);
            }
        }
        else
        {
            if(isOnFloor(self))
                self->walkDirection = 0.0f;
        }
    }
    else
    {
        self->walkDirection = 0.0f;
    }

    self->spriteFlipX = self->lookDirection.x < 0.0f;

    Super::update(self, delta);
}

//============================================================================
// EntityDonMeowthBehavior
//============================================================================

void EntityDonMeowthBehavior::spawn(Entity *self)
{
    Super::spawn(self);
    self->setExtent(Vector2F(1.8f, 0.6f));

    self->spriteSheet = &global.catDogsSprites;
    self->spriteIndex = Vector2I(0, 0);
    self->spriteOffset = Vector2F(-0.2f, 0.2 + -3*UnitsPerPixel);
}

//============================================================================
// EntityMrPresidentBehavior
//============================================================================

void EntityMrPresidentBehavior::spawn(Entity *self)
{
    Super::spawn(self);
}
