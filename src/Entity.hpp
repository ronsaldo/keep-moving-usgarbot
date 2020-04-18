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

class Renderer;
struct Entity
{
    // Metadata
    SmallFixedString<32> name;
    EntityBehaviorType type;

    // Size and collisions
    Vector2F halfExtent;

    // Mechanical attributes
    Vector2F position;
    Vector2F velocity;
    Vector2F acceleration;
    Vector2F dampening;

    // Visual attributes
    uint32_t color;

    Box2F boundingBox()
    {
        return Box2F::withCenterAndHalfExtent(position, halfExtent);
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

inline void Entity::spawn()
{
    entityBehaviorTypeIntoClass(type)->spawn(this);
}

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

class EntityCharacterBehavior : public EntityBehavior
{
public:
    virtual void update(Entity *self, float delta) override;

    virtual bool needsTicking(Entity *self) override
    {
        (void)self;
        return true;
    }
};

class EntityPlayerBehavior : public EntityCharacterBehavior
{
public:
    virtual void update(Entity *self, float delta) override;
};

class EntityCollisionBehavior : public EntityBehavior
{
public:
};

class EntityDoorBehavior : public EntityBehavior
{
public:
};


#endif //ENTITY_HPP
