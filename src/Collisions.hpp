#ifndef COLLISIONS_HPP
#define COLLISIONS_HPP

#include "Box2.hpp"
#include "FixedVector.hpp"
#define CollisionNotStuckEpsilon 0.0001f
#define CollisionSweepStopEpsilon 0.0001f

class Entity;

struct CollisionSweepTestResult
{
    CollisionSweepTestResult()
        : collisionDistance(INFINITY), hasCollision(false) {};

    float collisionDistance;
    bool hasCollision;
    Box2F collidingBox;
    Entity *collidingEntity;

    FixedVector<Entity*, 10> entityExclusionSet;
};

// Collision testing.
bool isBoxCollidingWithWorld(const Box2F &box);
bool isBoxCollidingWithSolid(const Box2F &box);

void sweepCollisionBoxAlongRayWithWorld(const Vector2F &boxHalfExtent, const Ray2F &ray, CollisionSweepTestResult &outResult);
void sweepCollisionBoxAlongRayWithCollidingEntities(const Vector2F &boxHalfExtent, const Ray2F &ray, CollisionSweepTestResult &outResult);

void sweepCollisionBoxAlongRay(const Vector2F &boxHalfExtent, const Ray2F &ray, CollisionSweepTestResult &outResult);

#endif //COLLISIONS_HPP
