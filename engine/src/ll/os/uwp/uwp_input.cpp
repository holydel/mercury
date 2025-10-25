#include "uwp_input.h"
#include "mercury_input.h"
#include "../../../input.h"
#include <windows.h>
#include <windowsx.h>
#include <cstring>
#include <unordered_map>

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
static POINT g_mousePosition = {0, 0};
static DWORD g_lastClickTime[5] = {0};
static POINT g_lastClickPosition[5] = {{0, 0}};

// Key mapping from Windows virtual key codes to Mercury Key enum
static std::unordered_map<WPARAM, Key> g_keyMap = {
    {'A', Key::A}, {'B', Key::B}, {'C', Key::C}, {'D', Key::D},
    {'E', Key::E}, {'F', Key::F}, {'G', Key::G}, {'H', Key::H},
    {'I', Key::I}, {'J', Key::J}, {'K', Key::K}, {'L', Key::L},
    {'M', Key::M}, {'N', Key::N}, {'O', Key::O}, {'P', Key::P},
    {'Q', Key::Q}, {'R', Key::R}, {'S', Key::S}, {'T', Key::T},
    {'U', Key::U}, {'V', Key::V}, {'W', Key::W}, {'X', Key::X},
    {'Y', Key::Y}, {'Z', Key::Z},
    {'0', Key::Num0}, {'1', Key::Num1}, {'2', Key::Num2},
    {'3', Key::Num3}, {'4', Key::Num4}, {'5', Key::Num5},
    {'6', Key::Num6}, {'7', Key::Num7}, {'8', Key::Num8},
    {'9', Key::Num9},
    {VK_F1, Key::F1}, {VK_F2, Key::F2}, {VK_F3, Key::F3}, {VK_F4, Key::F4},
    {VK_F5, Key::F5}, {VK_F6, Key::F6}, {VK_F7, Key::F7}, {VK_F8, Key::F8},
    {VK_F9, Key::F9}, {VK_F10, Key::F10}, {VK_F11, Key::F11}, {VK_F12, Key::F12},
    {VK_ESCAPE, Key::Escape}, {VK_TAB, Key::Tab}, {VK_SHIFT, Key::Shift},
    {VK_CONTROL, Key::Control}, {VK_MENU, Key::Alt}, {VK_LWIN, Key::Super},
    {VK_RWIN, Key::Super}, {VK_SPACE, Key::Space}, {VK_RETURN, Key::Enter},
    {VK_BACK, Key::Backspace}, {VK_DELETE, Key::Delete}
};

// Helper function to convert Key enum to array index
static int KeyToIndex(Key key) {
    return static_cast<int>(key);
}

// Helper function to convert MouseButton enum to array index
static int MouseButtonToIndex(MouseButton button) {
    return static_cast<int>(button);
}

// Helper function to get mouse button from Windows message
static MouseButton GetMouseButtonFromMessage(UINT message, WPARAM wParam) {
    switch (message) {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
            return MouseButton::Left;
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_RBUTTONDBLCLK:
            return MouseButton::Right;
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MBUTTONDBLCLK:
            return MouseButton::Middle;
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
        case WM_XBUTTONDBLCLK:
            return (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? MouseButton::X1 : MouseButton::X2;
        default:
            return MouseButton::Left;
    }
}

// Windows message handlers for input
void HandleKeyDown(WPARAM wParam, LPARAM lParam) {
    auto it = g_keyMap.find(wParam);
    if (it != g_keyMap.end()) {
        int index = KeyToIndex(it->second);
        if (index >= 0 && index < 256) {
            g_currentKeyStates[index] = true;
        }
    }
}

void HandleKeyUp(WPARAM wParam, LPARAM lParam) {
    auto it = g_keyMap.find(wParam);
    if (it != g_keyMap.end()) {
        int index = KeyToIndex(it->second);
        if (index >= 0 && index < 256) {
            g_currentKeyStates[index] = false;
        }
    }
}

void HandleChar(WPARAM wParam, LPARAM lParam) {
    if (wParam > 0 && wParam < 0x10000) {
        g_lastChar = static_cast<c32>(wParam);
    }
}

void HandleMouseButton(UINT message, WPARAM wParam, LPARAM lParam, bool isDown) {
    //MouseButton button = GetMouseButtonFromMessage(message, wParam);
    //int index = MouseButtonToIndex(button);
    //
    //if (index >= 0 && index < 5) {
    //    g_currentMouseStates[index] = isDown;
    //    
    //    // Handle double-click detection
    //    if (isDown) {
    //        POINT currentPos = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
    //        DWORD currentTime = GetTickCount();
    //        
    //        // Check if this is a potential double-click
    //        const DWORD DOUBLE_CLICK_TIME = GetDoubleClickTime();
    //        const int DOUBLE_CLICK_DISTANCE = 5; // pixels
    //        
    //        if ((currentTime - g_lastClickTime[index]) <= DOUBLE_CLICK_TIME &&
    //            abs(currentPos.x - g_lastClickPosition[index].x) <= DOUBLE_CLICK_DISTANCE &&
    //            abs(currentPos.y - g_lastClickPosition[index].y) <= DOUBLE_CLICK_DISTANCE) {
    //            g_mouseDoubleClicked[index] = true;
    //        }
    //        
    //        g_lastClickTime[index] = currentTime;
    //        g_lastClickPosition[index] = currentPos;
    //    }
    //}
}

void HandleMouseDoubleClick(UINT message, WPARAM wParam, LPARAM lParam) {
    MouseButton button = GetMouseButtonFromMessage(message, wParam);
    int index = MouseButtonToIndex(button);
    
    if (index >= 0 && index < 5) {
        g_mouseDoubleClicked[index] = true;
        g_currentMouseStates[index] = true;
    }
}

void HandleMouseMove(WPARAM wParam, LPARAM lParam) {
    //g_mousePosition.x = GET_X_LPARAM(lParam);
    //g_mousePosition.y = GET_Y_LPARAM(lParam);
}


void MercuryInputPreTick() {

}

void MercuryInputPostTick() {
    // Copy current states to previous for next frame's pressed/released detection
    memcpy(g_previousKeyStates, g_currentKeyStates, sizeof(g_currentKeyStates));
    memcpy(g_previousMouseStates, g_currentMouseStates, sizeof(g_currentMouseStates));
    
    // Reset per-frame states
    g_lastChar = 0;
    memset(g_mouseDoubleClicked, false, sizeof(g_mouseDoubleClicked));
}

// Keyboard class method implementations
bool Keyboard::IsKeyDown(Key key) const {
    int index = KeyToIndex(key);
    return (index >= 0 && index < 256) ? g_currentKeyStates[index] : false;
}

bool Keyboard::IsKeyUp(Key key) const {
    return !IsKeyDown(key);
}

bool Keyboard::IsKeyPressed(Key key) const {
    int index = KeyToIndex(key);
    if (index < 0 || index >= 256) return false;
    return g_currentKeyStates[index] && !g_previousKeyStates[index];
}

bool Keyboard::IsKeyReleased(Key key) const {
    int index = KeyToIndex(key);
    if (index < 0 || index >= 256) return false;
    return !g_currentKeyStates[index] && g_previousKeyStates[index];
}

c32 Keyboard::GetChar() const {
    return g_lastChar;
}

// Mouse class method implementations
bool Mouse::IsButtonDown(MouseButton button) const {
    int index = MouseButtonToIndex(button);
    return (index >= 0 && index < 5) ? g_currentMouseStates[index] : false;
}

bool Mouse::IsButtonUp(MouseButton button) const {
    return !IsButtonDown(button);
}

bool Mouse::IsButtonPressed(MouseButton button) const {
    int index = MouseButtonToIndex(button);
    if (index < 0 || index >= 5) return false;
    return g_currentMouseStates[index] && !g_previousMouseStates[index];
}

bool Mouse::IsButtonReleased(MouseButton button) const {
    int index = MouseButtonToIndex(button);
    if (index < 0 || index >= 5) return false;
    return !g_currentMouseStates[index] && g_previousMouseStates[index];
}

bool Mouse::IsButtonDoubleClicked(MouseButton button) const {
    int index = MouseButtonToIndex(button);
    return (index >= 0 && index < 5) ? g_mouseDoubleClicked[index] : false;
}

void Mouse::SetCursorType(CursorType type) const {
    g_currentCursorType = type;
    
    //HCURSOR cursor = nullptr;
    //switch (type) {
    //    case CursorType::Arrow: cursor = LoadCursor(nullptr, IDC_ARROW); break;
    //    case CursorType::IBeam: cursor = LoadCursor(nullptr, IDC_IBEAM); break;
    //    case CursorType::Hand: cursor = LoadCursor(nullptr, IDC_HAND); break;
    //    case CursorType::Crosshair: cursor = LoadCursor(nullptr, IDC_CROSS); break;
    //    case CursorType::Wait: cursor = LoadCursor(nullptr, IDC_WAIT); break;
    //    case CursorType::Help: cursor = LoadCursor(nullptr, IDC_HELP); break;
    //    case CursorType::ResizeHorizontal: cursor = LoadCursor(nullptr, IDC_SIZEWE); break;
    //    case CursorType::ResizeVertical: cursor = LoadCursor(nullptr, IDC_SIZENS); break;
    //    case CursorType::ResizeDiagonal: cursor = LoadCursor(nullptr, IDC_SIZENWSE); break;
    //    default: cursor = LoadCursor(nullptr, IDC_ARROW); break;
    //}
    //
    //if (cursor) {
    //    SetCursor(cursor);
    //}
}

CursorType Mouse::GetCursorType() const {
    return g_currentCursorType;
}

  float Gamepad::GetBatteryLevel() const
  {
    return 1.0f;
  }

  bool Gamepad::IsConnected() const
  {
    return false;
  }