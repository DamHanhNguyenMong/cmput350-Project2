#pragma once

#include <bitset>
#include <unordered_map>
#include <algorithm>
#include <cstdint>
#include <utility>

#include "MathUtil.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include "LocationComponent.h"
#include "SpriteComponent.h"

namespace ECSEngine {
const float kStarBounce = 0.8f;   // fraction of speed kept after a bounce
struct CollisionComponent {
    uint8_t staticEntity : 1;
    uint8_t doesDamage : 1;
    uint8_t onGround : 1;
    uint8_t onCeiling : 1;
    uint8_t onLeftWall : 1;
    uint8_t onRightWall : 1;
    Rect collisionBox;          // The bounding box for collision detection
    Rect previousCollisionBox;  // The previous bounding box for collision detection
    Point2D previousLocation;
};
        // Check whether two axis-aligned rectangles overlap
        inline bool RectRect(const Rect& a, const Rect& b) {
            return !(a.topLeft.x + a.width < b.topLeft.x ||
                    b.topLeft.x + b.width < a.topLeft.x ||
                    a.topLeft.y + a.height < b.topLeft.y ||
                    b.topLeft.y + b.height < a.topLeft.y);
        }
        // return the world-space collision box based on the entity's location and the sprite's relative bounds
        inline Rect WorldBox(const Rect& rel, const Point2D& loc) {
            return Rect{{loc.x + rel.topLeft.x, loc.y + rel.topLeft.y}, rel.width, rel.height};
        }
        // Move an entity and its collision box by the same amount
        inline void MoveOutOfCollision(LocationComponent& location, CollisionComponent& collision, float dx, float dy) {
            // Move the entity's location and its collision box by the specified amounts
            location.loc.x += dx;
            location.loc.y += dy;
        }

        // Resolve a moving entity against a static obstacle.
        inline void ResolveCollision(LocationComponent& location, MovementComponent& movement, CollisionComponent& moving, const CollisionComponent& obstacle, const Point2D& obstacleLoc, float bounce = 0.0f) {
            const Rect current = WorldBox(moving.collisionBox, location.loc);
            const Rect previous = WorldBox(moving.collisionBox, moving.previousLocation);
            const Rect wall = WorldBox(obstacle.collisionBox, obstacleLoc);

            const float currentRight = current.topLeft.x + current.width;
            const float currentBottom = current.topLeft.y + current.height;

            const float previousRight = previous.topLeft.x + previous.width;
            const float previousBottom = previous.topLeft.y + previous.height;

            const float wallRight = wall.topLeft.x + wall.width;
            const float wallBottom = wall.topLeft.y + wall.height;
            // Determine which side was hit using the previous position
            // The order also prioritizes collisions caused by crossing an edge

            // Landed on top of the wall: previous bottom was above the wall's top, and current bottom is below the wall's top
            if (previousBottom <= wall.topLeft.y && currentBottom >= wall.topLeft.y) {
                float dy = wall.topLeft.y - currentBottom;
                MoveOutOfCollision(location, moving, 0.0f, dy);

                movement.velocity.y *= -bounce; // Stop downward movement
                moving.onGround = true; // standing on the wall
            }
            // Hit the underside of the obstacle: previous top was below the wall's bottom, and current top is above the wall's bottom
            else if (previous.topLeft.y >= wallBottom && current.topLeft.y <= wallBottom) {
                float dy = wallBottom - current.topLeft.y;

                MoveOutOfCollision(location, moving, 0.0f, dy);

                movement.velocity.y *= -bounce;
                moving.onCeiling = true; // touch the bottom of the wall
            }
            // Hit the left side of the obstacle (obstacle is to the right)
            else if (previousRight <= wall.topLeft.x && currentRight >= wall.topLeft.x) {
                float dx = wall.topLeft.x - currentRight;
                MoveOutOfCollision(location, moving, dx, 0.0f);

                movement.velocity.x *= -bounce; // Reverse horizontal movement and apply bounce factor
                moving.onRightWall = true; // on the right side of the player
            }
            // Hit the right side of the obstacle (obstacle is to the left)
            else if (previous.topLeft.x >= wallRight && current.topLeft.x <= wallRight) {
                float dx = wallRight - current.topLeft.x;

                MoveOutOfCollision(location, moving, dx, 0.0f);

                movement.velocity.x *= -bounce;
                moving.onLeftWall = true;
            }
            // Fallback: the entity was already overlapping the obstacle
            // or the previous position did not identify the collision side
            else {
                float overlapLeft = currentRight - wall.topLeft.x; // How much the moving entity overlaps the left side of the wall
                float overlapRight = wallRight - current.topLeft.x; // How much the moving entity overlaps the right side of the wall
                float overlapTop = currentBottom - wall.topLeft.y; // How much the moving entity overlaps the top side of the wall
                float overlapBottom = wallBottom - current.topLeft.y; // How much the moving entity overlaps the bottom side of the wall

                float overlapX = std::min(overlapLeft, overlapRight); // The smallest horizontal overlap
                float overlapY = std::min(overlapTop, overlapBottom); // The smallest vertical overlap
                // If the horizontal overlap is smaller, resolve horizontally; otherwise, resolve vertically
                if (overlapX < overlapY) {
                    // Resolve horizontally
                    if (current.topLeft.x < wall.topLeft.x) {
                        MoveOutOfCollision(location, moving, -overlapX, 0.0f);
                        moving.onRightWall = true;
                    } else {
                        MoveOutOfCollision(location, moving, overlapX, 0.0f);
                        moving.onLeftWall = true;
                    }

                    movement.velocity.x *= -bounce;
                }
                else {
                    // Resolve vertically
                    if (current.topLeft.y < wall.topLeft.y) {
                        MoveOutOfCollision(location, moving, 0.0f, -overlapY);
                        moving.onGround = true;
                    } else {
                        MoveOutOfCollision(location, moving, 0.0f, overlapY);
                        moving.onCeiling = true;
                    }

                    movement.velocity.y *= -bounce;
                }
            }
        }
        // Separate two overlapping dynamic entities and swap their velocities along the collision axis
        inline void ResolveStarStar(LocationComponent& locA, MovementComponent& movA, CollisionComponent& a,
                                    LocationComponent& locB, MovementComponent& movB, CollisionComponent& b) {
            Rect ra = WorldBox(a.collisionBox, locA.loc);
            Rect rb = WorldBox(b.collisionBox, locB.loc);
            // overlapX and overlapY are the amounts of overlap of the two stars along the x and y axes, respectively
            float overlapX = std::min(ra.topLeft.x + ra.width, rb.topLeft.x + rb.width) - std::max(ra.topLeft.x, rb.topLeft.x);
            float overlapY = std::min(ra.topLeft.y + ra.height, rb.topLeft.y + rb.height) - std::max(ra.topLeft.y, rb.topLeft.y);
            if (overlapX <= 0 || overlapY <= 0)
                return;   // only touching, nothing to resolve
            // if the overlap along the x-axis is less than the overlap along the y-axis, resolve the collision along the x-axis; otherwise, resolve it along the y-axis
            if (overlapX < overlapY) {
                // if the left edge of A is to the left of the left edge of B, then A is to the left of B, 
                // so we push A to the left and B to the right; otherwise, we push A to the right and B to the left
                float dir = (ra.topLeft.x < rb.topLeft.x) ? -1.0f : 1.0f;   // direction A is pushed: -1 if A is to the left of B, 1 if A is to the right of B
                MoveOutOfCollision(locA, a, dir * overlapX / 2.0f, 0.0f); // move A out of collision by half the overlap distance
                MoveOutOfCollision(locB, b, -dir * overlapX / 2.0f, 0.0f); // move B out of collision by half the overlap distance in the opposite direction
                std::swap(movA.velocity.x, movB.velocity.x); // swap the x velocities
                movA.velocity.x *= kStarBounce; // apply bounce factor to the x velocities
                movB.velocity.x *= kStarBounce;
            } else {
                float dir = (ra.topLeft.y < rb.topLeft.y) ? -1.0f : 1.0f; // direction A is pushed: -1 if A is above B, 1 if A is below B
                MoveOutOfCollision(locA, a, 0.0f, dir * overlapY / 2.0f);
                MoveOutOfCollision(locB, b, 0.0f, -dir * overlapY / 2.0f);
                std::swap(movA.velocity.y, movB.velocity.y);
                movA.velocity.y *= kStarBounce;
                movB.velocity.y *= kStarBounce;
            }
        }

}  // namespace ECSEngine
