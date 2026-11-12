#pragma once

#include <bitset>
#include <unordered_map>
#include <algorithm>
#include <cstdint>

#include "MathUtil.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include "LocationComponent.h"
#include "SpriteComponent.h"

namespace ECSEngine {

struct CollisionComponent {
    uint8_t staticEntity : 1;
    uint8_t doesDamage : 1;
    uint8_t onGround : 1;
    uint8_t onCeiling : 1;
    uint8_t onLeftWall : 1;
    uint8_t onRightWall : 1;
    Rect collisionBox;          // The bounding box for collision detection
    Rect previousCollisionBox;  // The previous bounding box for collision detection
};
        // Check whether two axis-aligned rectangles overlap
        inline bool RectRect(const Rect& a, const Rect& b) {
            return !(a.topLeft.x + a.width < b.topLeft.x ||
                    b.topLeft.x + b.width < a.topLeft.x ||
                    a.topLeft.y + a.height < b.topLeft.y ||
                    b.topLeft.y + b.height < a.topLeft.y);
        }
        inline void UpdateCollisionBox( CollisionComponent& collision, const LocationComponent& location, const SpriteComponent& sprite) {
                collision.collisionBox.topLeft = {
                    location.loc.x + sprite.bounds.topLeft.x,
                    location.loc.y + sprite.bounds.topLeft.y
                };

                collision.collisionBox.width = sprite.bounds.width;
                collision.collisionBox.height = sprite.bounds.height;
        }
        // Move an entity and its collision box by the same amount
        inline void MoveOutOfCollision(LocationComponent& location, CollisionComponent& collision, float dx, float dy) {
            // Move the entity's location and its collision box by the specified amounts
            location.loc.x += dx;
            location.loc.y += dy;

            collision.collisionBox.topLeft.x += dx;
            collision.collisionBox.topLeft.y += dy;
        }
        // Resolve a moving entity against a static obstacle.
        inline void ResolveCollision(LocationComponent& location, MovementComponent& movement, CollisionComponent& moving, const CollisionComponent& obstacle) {
            const Rect& current = moving.collisionBox;
            const Rect& previous = moving.previousCollisionBox;
            const Rect& wall = obstacle.collisionBox;

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

                movement.velocity.y = 0.0f; // Stop downward movement
                moving.onGround = true; // standing on the wall
            }
            // Hit the underside of the obstacle: previous top was below the wall's bottom, and current top is above the wall's bottom
            else if (previous.topLeft.y >= wallBottom && current.topLeft.y <= wallBottom) {
                float dy = wallBottom - current.topLeft.y;

                MoveOutOfCollision(location, moving, 0.0f, dy);

                movement.velocity.y = 0.0f;
                moving.onCeiling = true; // touch the bottom of the wall
            }
            // Hit the left side of the obstacle (obstacle is to the right)
            else if (previousRight <= wall.topLeft.x && currentRight >= wall.topLeft.x) {
                float dx = wall.topLeft.x - currentRight;
                MoveOutOfCollision(location, moving, dx, 0.0f);

                movement.velocity.x = 0.0f;
                moving.onRightWall = true; // on the right side of the player
            }
            // Hit the right side of the obstacle (obstacle is to the left)
            else if (previous.topLeft.x >= wallRight && current.topLeft.x <= wallRight) {
                float dx = wallRight - current.topLeft.x;

                MoveOutOfCollision(location, moving, dx, 0.0f);

                movement.velocity.x = 0.0f;
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

                    movement.velocity.x = 0.0f;
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

                    movement.velocity.y = 0.0f;
                }
            }
        }

}  // namespace ECSEngine
