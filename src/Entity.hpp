#ifndef ENTITY_HPP
#define ENTITY_HPP

#include "Box2.hpp"
#include "FixedString.hpp"
#include "TileSet.hpp"
#include <stdint.h>

enum EntityBehaviorType : uint8_t {
#   define ENTITY_BEHAVIOR_TYPE(typeName) typeName,
#   include "EntityBehaviorTypes.inc"
#   undef ENTITY_BEHAVIOR_TYPE
};

#define HumanFallTerminalVelocity 53.0f

class Renderer;
struct MapEntityLayerState;

class Entity
{
private:
    bool isDead_; // Do not modify directly
public:
    // Metadata
    SmallFixedString<32> name;
    EntityBehaviorType type;

    bool isDead() const
    {
        return isDead_;
    }

    // Kill entities by putting them on a free list.
    void kill();

    // Size and collisions
    Vector2F halfExtent;

    // Mechanical attributes
    Vector2F position;
    Vector2F velocity;
    Vector2F acceleration;
    Vector2F damping;
    float mass;
    float inverseMass;
    float coefficientOfRestitution;

    // Specifics attributes.
    Vector2F lookDirection;
    Vector2F walkDirection;
    Entity *owner; // To not hurt oneself with a bullet
    float remainingLifeTime;
    float remainingInvincibilityTime;
    float remainingBulletReloadTime;
    float remainingActionTime;
    uint32_t hitPoints;
    int contactDamage;

    // Visual attributes
    uint32_t color;
    TileSet *spriteSheet;
    Vector2I spriteIndex;
    Vector2F spriteOffset;
    bool spriteFlipX;
    bool spriteFlipY;
    Box2F debugSensor;

    void applyImpulse(const Vector2F &impulse)
    {
        velocity += impulse*inverseMass;
    }

    void setMass(float newMass)
    {
        mass = newMass;
        inverseMass = 1.0f / newMass;
    }

    Box2F boundingBox()
    {
        return Box2F::withCenterAndHalfExtent(position, halfExtent);
    }

    void setExtent(const Vector2F &v)
    {
        halfExtent = v/2;
    }

    void computeDampingForTerminalVelocity(const Vector2F &terminalVelocity, const Vector2F &maxAcceleration)
    {
        damping = maxAcceleration / terminalVelocity;
    }

    void updateLookDirectionWithNormalizedVector(const Vector2F &v)
    {
        auto lastDirection = lookDirection;

        if(v.x >= 0.5f)
            lookDirection.x = 1.0f;
        else if(v.x <= -0.5f)
            lookDirection.x = -1.0f;
        else
            lookDirection.x = 0;

        if(v.y >= 0.5f)
            lookDirection.y = 1.0f;
        else if(v.y <= -0.5f)
            lookDirection.y = -1.0f;
        else
            lookDirection.y = 0;

        if(lookDirection.x == 0 && lookDirection.y == 0)
            lookDirection = lastDirection;
    }

    bool isInvincible()
    {
        return remainingInvincibilityTime > 0.0f;
    }

    void spawn();
    void update(float delta);
    void renderWith(Renderer &renderer);
    void hurtAt(float damage, const Vector2F &hitPoint, const Vector2F &hitImpulse);
    bool needsTicking();
    bool isPlayer();
    bool isVIP();
};

class EntityBehavior
{
public:
    virtual void spawn(Entity *self)
    {
        self->color = 0xff0000ff;
    }

    virtual void update(Entity *self, float delta)
    {
        (void)self;
        (void)delta;
    }

    virtual void renderWith(Entity *self, Renderer &renderer);

    virtual bool needsTicking(Entity *self)
    {
        (void)self;
        return false;
    }

    virtual bool hasCollisions(Entity *self)
    {
        (void)self;
        return false;
    }

    virtual void hurtAt(Entity *self, float damage, const Vector2F &hitPoint, const Vector2F &hitImpulse)
    {
        (void)self;
        (void)damage;
        (void)hitPoint;
        (void)hitImpulse;
    }

    virtual bool isPlayer()
    {
        return false;
    }

    virtual bool isVIP()
    {
        return false;
    }
};

EntityBehavior *entityBehaviorTypeIntoClass(EntityBehaviorType type);

inline bool Entity::needsTicking()
{
    return entityBehaviorTypeIntoClass(type)->needsTicking(this);
}

inline void Entity::update(float delta)
{
    entityBehaviorTypeIntoClass(type)->update(this, delta);
}

inline void Entity::renderWith(Renderer &renderer)
{
    entityBehaviorTypeIntoClass(type)->renderWith(this, renderer);
}

inline void Entity::hurtAt(float damage, const Vector2F &hitPoint, const Vector2F &hitImpulse)
{
    entityBehaviorTypeIntoClass(type)->hurtAt(this, damage, hitPoint, hitImpulse);
}

inline bool Entity::isPlayer()
{
    return entityBehaviorTypeIntoClass(type)->isPlayer();
}

inline bool Entity::isVIP()
{
    return entityBehaviorTypeIntoClass(type)->isVIP();
}

class EntityNullBehavior : public EntityBehavior
{
public:
};

class EntityKinematicCollidingBehavior : public EntityBehavior
{
public:
    void sweepCollidingAlongSegment(Entity *self, const Vector2F &segmentStartPoint, const Vector2F &segmentEndPoint, uint32_t maxDepth=5);

    virtual Vector2F myGravity()
    {
        return Vector2F(0.0f, -9.8f);
    }

    virtual Vector2F fallTerminalVelocity()
    {
        return Vector2F(0.0f, HumanFallTerminalVelocity);
    }

    virtual Vector2F myTerminalVelocity()
    {
        return 10.0f;
    }

    // This is an approximate time. A proper computation requires using the ODE.
    virtual Vector2F myTerminalVelocityAchivementTime()
    {
        return 0.1f;
    }

    virtual Vector2F myEngineAcceleration()
    {
        return myTerminalVelocity() / myTerminalVelocityAchivementTime();
    }

    virtual void update(Entity *self, float delta) override;

    virtual bool hasCollisions(Entity *self) override
    {
        (void)self;
        return true;
    }
};

class EntityBulletBehavior : public EntityBehavior
{
public:
    typedef EntityBehavior Super;

    virtual void spawn(Entity *self) override;
    virtual void update(Entity *self, float delta) override;

    virtual bool needsTicking(Entity *self) override
    {
        (void)self;
        return true;
    }
};

class EntityCharacterBehavior : public EntityKinematicCollidingBehavior
{
public:
    typedef EntityKinematicCollidingBehavior Super;

    virtual void die(Entity *self);
    virtual void spawn(Entity *self) override;
    virtual void update(Entity *self, float delta) override;
    virtual void hurtAt(Entity *self, float damage, const Vector2F &hitPoint, const Vector2F &hitImpulse) override;

    virtual float maximumJumpHeight()
    {
        return 7.0f;
    }

    virtual float maximumJumpHeightTime()
    {
        return 0.6f;
    }

    virtual Vector2F jumpVelocity()
    {
        // Computing the formula for this on paper.
        // 1) tmax = v/g 2)/ hmax =v^2/(2g)
        // Solved, gives the following:
        // Note: The actual jump height is smaller because of the linear damping.
        return Vector2F(0.0f, 2.0f*maximumJumpHeight()/maximumJumpHeightTime());
    }

    virtual Vector2F myGravity() override
    {
        auto t = maximumJumpHeightTime();
        return Vector2F(0.0f, -2.0f*maximumJumpHeight()/(t*t));
    }

    virtual Vector2F dashSpeed()
    {
        return Vector2F(50.0f, 0.0f);
    }

    virtual float currentAmmunitionSpeed(Entity *self)
    {
        (void)self;
        return 10.0f;
    }

    virtual float currentAmmunitionDuration(Entity *self)
    {
        (void)self;
        return 1.0f;
    }

    virtual int currentAmmunitionPower(Entity *self)
    {
        (void)self;
        return 5.0f;
    }

    virtual float defaultInvincibilityPeriodDuration()
    {
        return 0.033f; // Two frames
    }

    virtual float currentAmmunitionMass(Entity *self)
    {
        (void)self;
        return 0.150f;
    }

    virtual float forwardTestGap(Entity *self)
    {
        (void)self;
        return 0.5f;
    }

    virtual float currentBulletReloadTime(Entity *self)
    {
        (void)self;
        return 0.1f;
    }

    bool isOnFloor(Entity *self);
    bool hasFloorForward(Entity *self);
    bool hasWallForward(Entity *self);
    bool canJump(Entity *self);
    void jump(Entity *self);

    bool canDash(Entity *self);
    void dash(Entity *self);

    void shoot(Entity *self, float bulletSpeed, float bulletLifeTime, int currentAmmunitionPower, float bulletMass);
    void shoot(Entity *self);

    virtual bool needsTicking(Entity *self) override
    {
        (void)self;
        return true;
    }
};

class EntityPlayerBehavior : public EntityCharacterBehavior
{
public:
    typedef EntityCharacterBehavior Super;

    virtual void spawn(Entity *self) override;
    virtual void update(Entity *self, float delta) override;

    virtual bool isPlayer() override
    {
        return true;
    }
};

class EntityEnemyBehavior : public EntityCharacterBehavior
{
public:
    typedef EntityCharacterBehavior Super;

    virtual void spawn(Entity *self) override;
    virtual float maximumTargetSightDistance(Entity *self)
    {
        return 13.0f;
    }

    bool hasTargetOnSight(Entity *self, Entity *testTarget);
    bool hasSomeTargetOnSight(Entity *self);
};

class EntityEnemySentryBehavior : public EntityEnemyBehavior
{
public:
    typedef EntityEnemyBehavior Super;

    virtual float directionLookingTime()
    {
        return 1.0f;
    }

    virtual void spawn(Entity *self) override;
    virtual void update(Entity *self, float delta) override;
};

class EntityEnemyTurretBehavior : public EntityEnemyBehavior
{
public:
    typedef EntityEnemyBehavior Super;

    virtual void spawn(Entity *self) override;
    virtual void update(Entity *self, float delta) override;
};

class EntityEnemyPatrolBehavior : public EntityEnemyBehavior
{
public:
    typedef EntityEnemyBehavior Super;

    virtual Vector2F myTerminalVelocity() override
    {
        return 3.5f;
    }

    virtual void spawn(Entity *self) override;
    virtual void update(Entity *self, float delta) override;
};

class EntityEnemyPatrolDogBehavior : public EntityEnemyPatrolBehavior
{
public:
    typedef EntityEnemyPatrolBehavior Super;

    virtual void spawn(Entity *self) override;
};

class EntityEnemySentryDogBehavior : public EntityEnemySentryBehavior
{
public:
    typedef EntityEnemySentryBehavior Super;

    virtual void spawn(Entity *self) override;
};

class EntityScoltedVIPBehavior : public EntityCharacterBehavior
{
public:
    typedef EntityCharacterBehavior Super;

    virtual Vector2F myTerminalVelocity() override
    {
        return 5.0f;
    }

    virtual void spawn(Entity *self) override;
    virtual void update(Entity *self, float delta) override;

    virtual bool isVIP() override
    {
        return true;
    }
};

class EntityDonMeowthBehavior : public EntityScoltedVIPBehavior
{
public:
    typedef EntityScoltedVIPBehavior Super;

    virtual void spawn(Entity *self) override;
};

class EntityMrPresidentBehavior : public EntityScoltedVIPBehavior
{
public:
    typedef EntityScoltedVIPBehavior Super;

    virtual void spawn(Entity *self) override;
};

class EntityCollisionBehavior : public EntityBehavior
{
public:
};

class EntityDoorBehavior : public EntityBehavior
{
public:
};


Entity *instatiateEntityInLayer(MapEntityLayerState *entityLayer, EntityBehaviorType type);

#endif //ENTITY_HPP
