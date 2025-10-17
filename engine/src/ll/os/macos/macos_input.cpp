#include "ll/os.h"
#include "mercury_input.h"
#include "../../../input.h"
#include <iostream>
#include <unordered_map>
#include <cstring>

#ifdef MERCURY_LL_OS_MACOS
#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>

using namespace mercury;
using namespace mercury::input;

// Global state for input tracking
static bool g_currentKeyStates[256] = {false};
static bool g_previousKeyStates[256] = {false};
static bool g_currentMouseStates[5] = {false};
static bool g_previousMouseStates[5] = {false};
static bool g_mouseDoubleClicked[5] = {false};
static c32 g_lastChar = 0;
static CursorType g_currentCursorType = CursorType::Arrow;
static NSPoint g_mousePosition = {0, 0};
static NSTimeInterval g_lastClickTime[5] = {0};
static NSPoint g_lastClickPosition[5] = {{0, 0}};

// Key mapping from macOS key codes to Mercury Key enum
static std::unordered_map<unsigned short, Key> g_keyMap = {
    {kVK_ANSI_A, Key::A}, {kVK_ANSI_B, Key::B}, {kVK_ANSI_C, Key::C}, {kVK_ANSI_D, Key::D},
    {kVK_ANSI_E, Key::E}, {kVK_ANSI_F, Key::F}, {kVK_ANSI_G, Key::G}, {kVK_ANSI_H, Key::H},
    {kVK_ANSI_I, Key::I}, {kVK_ANSI_J, Key::J}, {kVK_ANSI_K, Key::K}, {kVK_ANSI_L, Key::L},
    {kVK_ANSI_M, Key::M}, {kVK_ANSI_N, Key::N}, {kVK_ANSI_O, Key::O}, {kVK_ANSI_P, Key::P},
    {kVK_ANSI_Q, Key::Q}, {kVK_ANSI_R, Key::R}, {kVK_ANSI_S, Key::S}, {kVK_ANSI_T, Key::T},
    {kVK_ANSI_U, Key::U}, {kVK_ANSI_V, Key::V}, {kVK_ANSI_W, Key::W}, {kVK_ANSI_X, Key::X},
    {kVK_ANSI_Y, Key::Y}, {kVK_ANSI_Z, Key::Z},
    {kVK_ANSI_0, Key::Num0}, {kVK_ANSI_1, Key::Num1}, {kVK_ANSI_2, Key::Num2},
    {kVK_ANSI_3, Key::Num3}, {kVK_ANSI_4, Key::Num4}, {kVK_ANSI_5, Key::Num5},
    {kVK_ANSI_6, Key::Num6}, {kVK_ANSI_7, Key::Num7}, {kVK_ANSI_8, Key::Num8},
    {kVK_ANSI_9, Key::Num9},
    {kVK_F1, Key::F1}, {kVK_F2, Key::F2}, {kVK_F3, Key::F3}, {kVK_F4, Key::F4},
    {kVK_F5, Key::F5}, {kVK_F6, Key::F6}, {kVK_F7, Key::F7}, {kVK_F8, Key::F8},
    {kVK_F9, Key::F9}, {kVK_F10, Key::F10}, {kVK_F11, Key::F11}, {kVK_F12, Key::F12},
    {kVK_Escape, Key::Escape}, {kVK_Tab, Key::Tab}, {kVK_Shift, Key::Shift},
    {kVK_Control, Key::Control}, {kVK_Option, Key::Alt}, {kVK_Command, Key::Super},
    {kVK_Space, Key::Space}, {kVK_Return, Key::Enter},
    {kVK_Delete, Key::Backspace}, {kVK_ForwardDelete, Key::Delete}
};

// Helper function to convert Key enum to array index
static int KeyToIndex(Key key) {
    return static_cast<int>(key);
}

// Helper function to convert mouse button to array index
static int MouseButtonToIndex(MouseButton button) {
    switch (button) {
        case MouseButton::Left:   return 0;
        case MouseButton::Right:  return 1;
        case MouseButton::Middle: return 2;
        case MouseButton::X1:     return 3;
        case MouseButton::X2:     return 4;
        default:                  return 0;
    }
}

// Update input state (called at the beginning of each frame)
void MercuryInputPreTick() {
    // Copy current states to previous states
    memcpy(g_previousKeyStates, g_currentKeyStates, sizeof(g_currentKeyStates));
    memcpy(g_previousMouseStates, g_currentMouseStates, sizeof(g_currentMouseStates));
    
    // Reset double-click flags
    memset(g_mouseDoubleClicked, false, sizeof(g_mouseDoubleClicked));
}

// Update input state (called at the end of each frame)
void MercuryInputPostTick() {
    // Reset last character
    g_lastChar = 0;
}

// Handle key down event
void HandleKeyDown(unsigned short keyCode) {
    auto it = g_keyMap.find(keyCode);
    if (it != g_keyMap.end()) {
        int index = KeyToIndex(it->second);
        g_currentKeyStates[index] = true;
    }
}

// Handle key up event
void HandleKeyUp(unsigned short keyCode) {
    auto it = g_keyMap.find(keyCode);
    if (it != g_keyMap.end()) {
        int index = KeyToIndex(it->second);
        g_currentKeyStates[index] = false;
    }
}

// Handle character input
void HandleChar(c32 character) {
    g_lastChar = character;
}

// Handle mouse button down
void HandleMouseButtonDown(MouseButton button, NSPoint position) {
    int index = MouseButtonToIndex(button);
    g_currentMouseStates[index] = true;
    
    // Check for double-click
    NSTimeInterval currentTime = [NSDate timeIntervalSinceReferenceDate];
    NSTimeInterval timeDiff = currentTime - g_lastClickTime[index];
    NSPoint posDiff = NSMakePoint(position.x - g_lastClickPosition[index].x, 
                                  position.y - g_lastClickPosition[index].y);
    float distance = sqrt(posDiff.x * posDiff.x + posDiff.y * posDiff.y);
    
    if (timeDiff < 0.5 && distance < 10.0) { // 500ms and 10 pixels threshold
        g_mouseDoubleClicked[index] = true;
    }
    
    g_lastClickTime[index] = currentTime;
    g_lastClickPosition[index] = position;
}

// Handle mouse button up
void HandleMouseButtonUp(MouseButton button) {
    int index = MouseButtonToIndex(button);
    g_currentMouseStates[index] = false;
}

// Handle mouse move
void HandleMouseMove(NSPoint position) {
    g_mousePosition = position;
}

// Keyboard implementation
bool Keyboard::IsKeyDown(Key key) const {
    int index = KeyToIndex(key);
    return g_currentKeyStates[index];
}

bool Keyboard::IsKeyUp(Key key) const {
    int index = KeyToIndex(key);
    return !g_currentKeyStates[index];
}

bool Keyboard::IsKeyPressed(Key key) const {
    int index = KeyToIndex(key);
    return g_currentKeyStates[index] && !g_previousKeyStates[index];
}

bool Keyboard::IsKeyReleased(Key key) const {
    int index = KeyToIndex(key);
    return !g_currentKeyStates[index] && g_previousKeyStates[index];
}

c32 Keyboard::GetChar() const {
    return g_lastChar;
}

// Mouse implementation
bool Mouse::IsButtonDown(MouseButton button) const {
    int index = MouseButtonToIndex(button);
    return g_currentMouseStates[index];
}

bool Mouse::IsButtonUp(MouseButton button) const {
    int index = MouseButtonToIndex(button);
    return !g_currentMouseStates[index];
}

bool Mouse::IsButtonPressed(MouseButton button) const {
    int index = MouseButtonToIndex(button);
    return g_currentMouseStates[index] && !g_previousMouseStates[index];
}

bool Mouse::IsButtonReleased(MouseButton button) const {
    int index = MouseButtonToIndex(button);
    return !g_currentMouseStates[index] && g_previousMouseStates[index];
}

bool Mouse::IsButtonDoubleClicked(MouseButton button) const {
    int index = MouseButtonToIndex(button);
    return g_mouseDoubleClicked[index];
}

void Mouse::SetCursorType(CursorType type) const {
    if (g_currentCursorType == type) return;
    
    g_currentCursorType = type;
    
    NSCursor* cursor = nil;
    switch (type) {
        case CursorType::Arrow:           cursor = [NSCursor arrowCursor]; break;
        case CursorType::IBeam:           cursor = [NSCursor IBeamCursor]; break;
        case CursorType::Hand:            cursor = [NSCursor pointingHandCursor]; break;
        case CursorType::Crosshair:       cursor = [NSCursor crosshairCursor]; break;
        case CursorType::Wait:            cursor = [NSCursor arrowCursor]; break; // No wait cursor in NSCursor
        case CursorType::Help:            cursor = [NSCursor arrowCursor]; break; // No help cursor in NSCursor
        case CursorType::ResizeHorizontal: cursor = [NSCursor resizeLeftRightCursor]; break;
        case CursorType::ResizeVertical:   cursor = [NSCursor resizeUpDownCursor]; break;
        case CursorType::ResizeDiagonal:   cursor = [NSCursor arrowCursor]; break; // No diagonal resize in NSCursor
        default:                          cursor = [NSCursor arrowCursor]; break;
    }
    
    if (cursor) {
        [cursor set];
    }
}

CursorType Mouse::GetCursorType() const {
    return g_currentCursorType;
}

// Gamepad stubs for macOS to ensure linkage and basic UI behavior
float Gamepad::GetBatteryLevel() const {
    return 1.0f; // Assume full battery when no native API available
}

bool Gamepad::IsConnected() const {
    return false; // Default to not connected on macOS unless implemented
}

#endif
