#include "WindowManager.h"

#include <algorithm>
#include <cassert>
#include <cstdint>

namespace ECSEngine {

WindowManager::WindowManager(unsigned int width, unsigned int height, const std::string& title)
    : mWindow(std::make_shared<sf::RenderWindow>(sf::VideoMode({width, height}), title)) {}

std::shared_ptr<sf::RenderWindow> WindowManager::GetWindow() const { return mWindow; }

Rect WindowManager::GetWindowBounds() const { 
    sf::Vector2u size = mWindow->getSize();
    return Rect{Point2D(0, 0), static_cast<float>(size.x), static_cast<float>(size.y)};
}

void WindowManager::DebugDrawRect(const Rect& r, int red, int green, int blue, int alpha) {
    // Convert the world rectangle into screen coordinates
    Rect screenRect = WorldToWindow(r);
    // Create a rectangle shape for debugging
    sf::RectangleShape rectangle(sf::Vector2f(screenRect.width, screenRect.height));
    rectangle.setPosition(screenRect.topLeft.x, screenRect.topLeft.y);
    rectangle.setFillColor(sf::Color(
        static_cast<std::uint8_t>(red),
        static_cast<std::uint8_t>(green),
        static_cast<std::uint8_t>(blue),
        static_cast<std::uint8_t>(alpha)
    ));

    mWindow->draw(rectangle);
}

void WindowManager::DebugFrameRect(const Rect& r, int red, int green, int blue, int alpha) {
    // Convert the world rectangle into screen coordinates
    Rect screenRect = WorldToWindow(r);
    // Create a rectangle shape for debugging
    sf::RectangleShape rectangle(sf::Vector2f(screenRect.width, screenRect.height));
    rectangle.setPosition(screenRect.topLeft.x, screenRect.topLeft.y);
    rectangle.setFillColor(sf::Color::Transparent);
    rectangle.setOutlineColor(sf::Color(
        static_cast<std::uint8_t>(red),
        static_cast<std::uint8_t>(green),
        static_cast<std::uint8_t>(blue),
        static_cast<std::uint8_t>(alpha)
    ));
    rectangle.setOutlineThickness(1.0f); // Set the outline thickness
    mWindow->draw(rectangle);
}

Rect WindowManager::WindowToWorld(const Rect& rect) const { 
    Point2D topLeft = WindowToWorld(rect.topLeft);
    return Rect{topLeft, rect.width / mWorldUnitsPerPixel, rect.height / mWorldUnitsPerPixel};
}

Point2D WindowManager::WindowToWorld(const Point2D& pt) const { 
    return Point2D(WindowToWorld(pt.x), WindowToWorld(pt.y));
}

Rect WindowManager::WorldToWindow(const Rect& rect) const { 
    Point2D topLeft = WorldToWindow(rect.topLeft);
    return Rect{topLeft, rect.width / mWorldUnitsPerPixel, rect.height / mWorldUnitsPerPixel};
}

Point2D WindowManager::WorldToWindow(const Point2D& pt) const { 
    return Point2D(WorldToWindow(pt.x), WorldToWindow(pt.y));
}

// choose a world point and the screen point where it should appear
void WindowManager::SetCamera(const Point2D& worldPt, const Point2D& screenPt) {
    Rect bounds = GetWindowBounds();

    float screenCenterX = bounds.width / 2.0f;
    float screenCenterY = bounds.height / 2.0f;

    // Calculate the world center based on the desired world point and screen point
    mWorldCenter.x = worldPt.x - (screenPt.x - screenCenterX) * mWorldUnitsPerPixel;
    mWorldCenter.y = worldPt.y - (screenPt.y - screenCenterY) * mWorldUnitsPerPixel;
}

Point2D WindowManager::GetWorldCenter() const { 
    return mWorldCenter;
}

float WindowManager::GetWorldScale() const { 
    return mWorldUnitsPerPixel;
}

void WindowManager::SetWorldCenter(const Point2D& worldCenter) {
    mWorldCenter = worldCenter;
}

void WindowManager::SetWorldScale(float worldUnitsPerPixel) {
    assert(worldUnitsPerPixel > 0);
    mWorldUnitsPerPixel = worldUnitsPerPixel;
}

// Specify the portion of the world that the camera should display
void WindowManager::SetWorldRect(const Rect& worldCameraRect) {
    // Window size in pixels
    Rect bounds = GetWindowBounds();

    assert(worldCameraRect.width > 0);
    assert(worldCameraRect.height > 0);
    // The world's center is the center of the specified camera rectangle
    mWorldCenter = Point2D( worldCameraRect.topLeft.x + worldCameraRect.width / 2.0f, worldCameraRect.topLeft.y + worldCameraRect.height / 2.0f);
    // Calculate the scale based on the ratio of world units to window pixels
    float scaleX = worldCameraRect.width / bounds.width;
    float scaleY = worldCameraRect.height / bounds.height;
    // Since we want the entire requested world rectangle to fit inside the window
    // choosing the larger scale ensures that the entire world rectangle is visible
    mWorldUnitsPerPixel = std::max(scaleX, scaleY);
}

float WindowManager::WindowToWorld(float x) const { 
    return (x - GetWindowBounds().width / 2.0f) * mWorldUnitsPerPixel + mWorldCenter.x;
}

float WindowManager::WorldToWindow(float x) const { 
    return (x - mWorldCenter.x) / mWorldUnitsPerPixel + GetWindowBounds().width / 2.0f;
}

}  // namespace ECSEngine
