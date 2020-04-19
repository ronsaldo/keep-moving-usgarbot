#include "Collisions.hpp"
#include "GameLogic.hpp"
#include "MapTransientState.hpp"

bool isWorldBoxCollidingWithMapTileLayer(const Box2F &box, const MapFileTileLayer *tileLayer)
{
    auto tileLayerExtent = tileLayer->extent;
    auto tileExtent = global.mainTileSet.tileExtent;
    auto boxInTileSpace = tileLayer->boxFromWorldIntoTileSpace(box, tileExtent.asVector2F());
    auto tileGridBox = boxInTileSpace.asBoundingIntegerBox().intersectionWithBox(tileLayer->tileGridBounds());

    auto sourceRow = tileLayer->tiles + tileGridBox.min.y*tileLayerExtent.x + tileGridBox.min.x;
    for(int32_t ty = tileGridBox.min.y; ty < tileGridBox.max.y; ++ty)
    {
        auto source = sourceRow;
        for(int32_t tx = tileGridBox.min.x; tx < tileGridBox.max.x; ++tx)
        {
            auto tileIndex = *source++;
            if(tileIndex == 0)
                continue;

            auto tileBox = Box2F(Vector2F(tx, ty), Vector2F(tx + 1, ty + 1));
            if(boxInTileSpace.intersectsWithBox(tileBox))
                return true;
        }

        sourceRow += tileLayerExtent.x;
    }
    return false;
}

bool isBoxCollidingWithWorld(const Box2F &box)
{
    auto mapState = global.mapTransientState;
    if(!mapState)
        return false;

    for(auto layer : mapState->layers)
    {
        if(layer->type != MapLayerType::Solid)
            continue;

        auto solidLayer = reinterpret_cast<MapSolidLayerState*> (layer);
        if(!solidLayer->mapTileLayer->isSolid())
            continue;

        if(isWorldBoxCollidingWithMapTileLayer(box, solidLayer->mapTileLayer))
            return true;
    }
    return false;
}

bool isBoxCollidingWithSolidEntity(const Box2F &box, const std::unordered_set<Entity*> &exclusionSet)
{
    auto mapState = global.mapTransientState;
    if(!mapState)
        return false;

    for(auto entity : mapState->collisionEntities)
    {
        if(exclusionSet.find(entity) != exclusionSet.end())
            continue;

        if(entity->boundingBox().intersectsWithBox(box))
            return true;
    }

    return false;
}

bool isBoxCollidingWithSolid(const Box2F &box, const std::unordered_set<Entity*> &exclusionSet)
{
    return isBoxCollidingWithWorld(box) || isBoxCollidingWithSolidEntity(box, exclusionSet);
}

bool isBoxCollidingWithSolid(const Box2F &box)
{
    return isBoxCollidingWithSolid(box, std::unordered_set<Entity*> ());
}

void sweepCollisionBoxAlongRayWithWorldWithMapTileLayer(const Vector2F &boxHalfExtent, const Ray2F &ray, const MapFileTileLayer *tileLayer, CollisionSweepTestResult &outResult)
{
    auto tileLayerExtent = tileLayer->extent;
    auto tileExtent = global.mainTileSet.tileExtent.asVector2F();

    auto tileRay = tileLayer->rayFromWorldIntoTileSpace(ray, tileExtent);
    auto tileRayStartPoint = tileRay.startPoint();
    auto tileRayEndPoint = tileRay.endPoint();

    auto extraTileHalfExtent = tileLayer->vectorFromWorldIntoTileSpace(boxHalfExtent, tileExtent);

    auto tileRayBoundingBox = Box2F(std::min(tileRayStartPoint, tileRayEndPoint), std::max(tileRayStartPoint, tileRayEndPoint)).grownWithHalfExtent(extraTileHalfExtent);
    auto tileGridBox = tileRayBoundingBox.asBoundingIntegerBox().intersectionWithBox(tileLayer->tileGridBounds());

    //printf("ray %f %f -> %f %f\n", ray.origin.x, ray.origin.y, ray.direction.x, ray.direction.y);
    //printf("tileRay %f %f -> %f %f\n", tileRay.origin.x, tileRay.origin.y, tileRay.direction.x, tileRay.direction.y);

    //printf("extraTileHalfExtent %f %f\n", extraTileHalfExtent.x, extraTileHalfExtent.y);
    //printf("tileRay %f %f %f %f\n", tileRayStartPoint.x, tileRayStartPoint.y, tileRayEndPoint.x, tileRayEndPoint.y);
    //printf("tileRayBoundingBox %f %f %f %f\n", tileRayBoundingBox.min.x, tileRayBoundingBox.min.y, tileRayBoundingBox.max.x, tileRayBoundingBox.max.y);
    //printf("tileGridBox %d %d %d %d\n", tileGridBox.min.x, tileGridBox.min.y, tileGridBox.max.x, tileGridBox.max.y);

    auto sourceRow = tileLayer->tiles + tileGridBox.min.y*tileLayerExtent.x + tileGridBox.min.x;
    for(int32_t ty = tileGridBox.min.y; ty < tileGridBox.max.y; ++ty)
    {
        auto source = sourceRow;
        for(int32_t tx = tileGridBox.min.x; tx < tileGridBox.max.x; ++tx)
        {
            auto tileIndex = *source++;
            if(tileIndex == 0)
                continue;

            auto tileBox = Box2F(Vector2F(tx, ty), Vector2F(tx + 1, ty + 1)).grownWithHalfExtent(extraTileHalfExtent);
            auto intersectionResult = tileBox.intersectionWithRay(tileRay);
            //printf("Test tile box: %f %f %f %f: %d %f\n", tileBox.min.x, tileBox.min.y, tileBox.max.x, tileBox.max.y, intersectionResult.first, intersectionResult.second);
            if(intersectionResult.first)
            {
                auto intersectionPoint = tileRay.pointAtT(intersectionResult.second);
                auto intersectionWorldPoint = tileLayer->pointFromTileIntoWorldSpace(intersectionPoint, tileExtent);
                auto intersectionWorldT = ray.direction.dot(intersectionWorldPoint - ray.origin);

                if(!outResult.hasCollision || intersectionWorldT < outResult.collisionDistance)
                {
                    //printf("Intersection point: %f %f\n", intersectionPoint.x, intersectionPoint.y);
                    //printf("intersectionWorldPoint: %f %f\n", intersectionWorldPoint.x, intersectionWorldPoint.y);
                    //printf("intersectionWorldT: %f\n", intersectionWorldT);

                    outResult.hasCollision = true;
                    outResult.collisionDistance = intersectionWorldT;
                    outResult.collidingEntity = nullptr;
                    outResult.collidingBox = tileLayer->boxFromTileIntoWorldSpace(tileBox, tileExtent);
                }

            }
        }

        sourceRow += tileLayerExtent.x;
    }
}

void sweepCollisionBoxAlongRayWithWorld(const Vector2F &boxHalfExtent, const Ray2F &ray, CollisionSweepTestResult &outResult)
{
    auto mapState = global.mapTransientState;
    if(!mapState)
        return;

    for(auto layer : mapState->layers)
    {
        if(layer->type != MapLayerType::Solid)
            continue;

        auto solidLayer = reinterpret_cast<MapSolidLayerState*> (layer);
        if(!solidLayer->mapTileLayer->isSolid())
            continue;

        sweepCollisionBoxAlongRayWithWorldWithMapTileLayer(boxHalfExtent, ray, solidLayer->mapTileLayer, outResult);
    }
}

void sweepCollisionBoxAlongRayWithCollidingEntities(const Vector2F &boxHalfExtent, const Ray2F &ray, CollisionSweepTestResult &outResult)
{
    auto mapState = global.mapTransientState;
    if(!mapState)
        return;

    for(auto entity : mapState->collisionEntities)
    {
        if(outResult.entityExclusionSet.includes(entity))
            continue;

        auto testBox = entity->boundingBox().grownWithHalfExtent(boxHalfExtent);
        auto intersectionResult = testBox.intersectionWithRay(ray);
        if(intersectionResult.first && (!outResult.hasCollision || intersectionResult.second < outResult.collisionDistance))
        {
            outResult.hasCollision = true;
            outResult.collisionDistance = intersectionResult.second;
            outResult.collidingBox = testBox;
            outResult.collidingEntity = entity;
        }
    }
}

void sweepCollisionBoxAlongRay(const Vector2F &boxHalfExtent, const Ray2F &ray, CollisionSweepTestResult &outResult)
{
    sweepCollisionBoxAlongRayWithWorld(boxHalfExtent, ray, outResult);
    sweepCollisionBoxAlongRayWithCollidingEntities(boxHalfExtent, ray, outResult);
}
