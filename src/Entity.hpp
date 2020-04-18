#ifndef ENTITY_HPP
#define ENTITY_HPP

#include "Box2.hpp"
#include "FixedString.hpp"
#include <stdint.h>

enum EntityBehaviorType : uint8_t {
#   define ENTITY_BEHAVIOR_TYPE(typeName) typeName,
#   include "EntityBehaviorTypes.inc"
#   undef ENTITY_BEHAVIOR_TYPE
};

#define HumalFallTerminalVelocity 53.0f

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

    // Specifics attributes.
    Vector2F lookDirection;
    Vector2F walkDirection;
    Entity *owner; // To not hurt oneself with a bullet
    float remainingLifeTime;
    uint32_t hitPoints;
    int contactDamage;

    // Visual attributes
    uint32_t color;

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

    void spawn();
    void update(float delta);
    void renderWith(Renderer &renderer);
    bool needsTicking();
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
        return Vector2F(0.0f, HumalFallTerminalVelocity);
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

    virtual void spawn(Entity *self) override;
    virtual void update(Entity *self, float delta) override;

    virtual Vector2F jumpVelocity()
    {
        return Vector2F(0.0f, 10.0f);
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

    bool isOnFloor(Entity *self);
    bool canJump(Entity *self);
    void jump(Entity *self);
    void shoot(Entity *self, float bulletSpeed, float bulletLifeTime, int currentAmmunitionPower);
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
};

class EntityEnemyBehavior : public EntityCharacterBehavior
{
public:
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
