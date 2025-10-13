#include "android_input.h"
#include "mercury_input.h"

void MercuryInputPreTick()
{

}

void MercuryInputPostTick()
{

}

using namespace mercury;
using namespace mercury::input;

// Keyboard class method implementations
bool Keyboard::IsKeyDown(Key key) const {
	return false;
}

bool Keyboard::IsKeyUp(Key key) const {
    return false;
}

bool Keyboard::IsKeyPressed(Key key) const {
    return false;
}

bool Keyboard::IsKeyReleased(Key key) const {
    return false;
}

c32 Keyboard::GetChar() const {
    return false;
}

// Mouse class method implementations
bool Mouse::IsButtonDown(MouseButton button) const {
    return false;
}

bool Mouse::IsButtonUp(MouseButton button) const {
    return !IsButtonDown(button);
}

bool Mouse::IsButtonPressed(MouseButton button) const {
    return false;
}

bool Mouse::IsButtonReleased(MouseButton button) const {
    return false;
}

bool Mouse::IsButtonDoubleClicked(MouseButton button) const {
    return false;
}

void Mouse::SetCursorType(CursorType type) const {
    
}

CursorType Mouse::GetCursorType() const {
    return CursorType::Arrow;
}

float Gamepad::GetBatteryLevel() const
{
    return 1.0f;
}

bool Gamepad::IsConnected() const
{
    return false;
}