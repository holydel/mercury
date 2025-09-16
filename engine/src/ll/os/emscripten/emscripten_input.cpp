#include "emscripten_input.h"
#include "mercury_input.h"
#include <emscripten.h>
#include <emscripten/html5.h>
#include <cstring>
#include <unordered_map>
#include <string>
#include "../../../input.h"

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

// Key mapping from HTML key codes to Mercury Key enum
static std::unordered_map<std::string, Key> g_keyMap = {
    {"KeyA", Key::A}, {"KeyB", Key::B}, {"KeyC", Key::C}, {"KeyD", Key::D},
    {"KeyE", Key::E}, {"KeyF", Key::F}, {"KeyG", Key::G}, {"KeyH", Key::H},
    {"KeyI", Key::I}, {"KeyJ", Key::J}, {"KeyK", Key::K}, {"KeyL", Key::L},
    {"KeyM", Key::M}, {"KeyN", Key::N}, {"KeyO", Key::O}, {"KeyP", Key::P},
    {"KeyQ", Key::Q}, {"KeyR", Key::R}, {"KeyS", Key::S}, {"KeyT", Key::T},
    {"KeyU", Key::U}, {"KeyV", Key::V}, {"KeyW", Key::W}, {"KeyX", Key::X},
    {"KeyY", Key::Y}, {"KeyZ", Key::Z},
    {"Digit0", Key::Num0}, {"Digit1", Key::Num1}, {"Digit2", Key::Num2},
    {"Digit3", Key::Num3}, {"Digit4", Key::Num4}, {"Digit5", Key::Num5},
    {"Digit6", Key::Num6}, {"Digit7", Key::Num7}, {"Digit8", Key::Num8},
    {"Digit9", Key::Num9},
    {"F1", Key::F1}, {"F2", Key::F2}, {"F3", Key::F3}, {"F4", Key::F4},
    {"F5", Key::F5}, {"F6", Key::F6}, {"F7", Key::F7}, {"F8", Key::F8},
    {"F9", Key::F9}, {"F10", Key::F10}, {"F11", Key::F11}, {"F12", Key::F12},
    {"Escape", Key::Escape}, {"Tab", Key::Tab}, {"ShiftLeft", Key::Shift},
    {"ShiftRight", Key::Shift}, {"ControlLeft", Key::Control}, 
    {"ControlRight", Key::Control}, {"AltLeft", Key::Alt}, {"AltRight", Key::Alt},
    {"MetaLeft", Key::Super}, {"MetaRight", Key::Super}, {"Space", Key::Space},
    {"Enter", Key::Enter}, {"Backspace", Key::Backspace}, {"Delete", Key::Delete}
};

// Helper function to convert Key enum to array index
static int KeyToIndex(Key key) {
    return static_cast<int>(key);
}

// Helper function to convert MouseButton enum to array index
static int MouseButtonToIndex(MouseButton button) {
    return static_cast<int>(button);
}

// Keyboard event callbacks
static EM_BOOL OnKeyDown(int eventType, const EmscriptenKeyboardEvent* e, void* userData) {
    auto it = g_keyMap.find(e->code);
    if (it != g_keyMap.end()) {
        int index = KeyToIndex(it->second);
        if (index >= 0 && index < 256) {
            g_currentKeyStates[index] = true;
        }
    }
    return EM_TRUE;
}

static EM_BOOL OnKeyUp(int eventType, const EmscriptenKeyboardEvent* e, void* userData) {
    auto it = g_keyMap.find(e->code);
    if (it != g_keyMap.end()) {
        int index = KeyToIndex(it->second);
        if (index >= 0 && index < 256) {
            g_currentKeyStates[index] = false;
        }
    }
    return EM_TRUE;
}

static EM_BOOL OnKeyPress(int eventType, const EmscriptenKeyboardEvent* e, void* userData) {
    if (e->charCode > 0) {
        g_lastChar = static_cast<c32>(e->charCode);
    }
    return EM_TRUE;
}

// Mouse event callbacks
static EM_BOOL OnMouseDown(int eventType, const EmscriptenMouseEvent* e, void* userData) {
    if (e->button >= 0 && e->button < 5) {
        g_currentMouseStates[e->button] = true;
    }
    return EM_TRUE;
}

static EM_BOOL OnMouseUp(int eventType, const EmscriptenMouseEvent* e, void* userData) {
    if (e->button >= 0 && e->button < 5) {
        g_currentMouseStates[e->button] = false;
    }
    return EM_TRUE;
}

static EM_BOOL OnMouseDoubleClick(int eventType, const EmscriptenMouseEvent* e, void* userData) {
    if (e->button >= 0 && e->button < 5) {
        g_mouseDoubleClicked[e->button] = true;
    }
    return EM_TRUE;
}

// Pre-tick function - called before application tick
// This ensures the most current input state is available during app tick
void MercuryInputPreTick() {
    // Nothing to do here for Emscripten - input events are processed immediately
    // during browser event callbacks
}

// Post-tick function - called after application tick
// This handles the state copying for pressed/released detection
void MercuryInputPostTick() {
    // Copy current states to previous for next frame's pressed/released detection
    memcpy(g_previousKeyStates, g_currentKeyStates, sizeof(g_currentKeyStates));
    memcpy(g_previousMouseStates, g_currentMouseStates, sizeof(g_currentMouseStates));
    
    // Reset per-frame states
    g_lastChar = 0;
    memset(g_mouseDoubleClicked, false, sizeof(g_mouseDoubleClicked));
}

void RegisterEmscriptenInputCallbacks() {
    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, 1, OnKeyDown);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, 1, OnKeyUp);
    emscripten_set_keypress_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, 1, OnKeyPress);
    
    emscripten_set_mousedown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, 1, OnMouseDown);
    emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, 1, OnMouseUp);
    emscripten_set_dblclick_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, 1, OnMouseDoubleClick);
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
    
    const char* cursorStyle = "default";
    switch (type) {
        case CursorType::Arrow: cursorStyle = "default"; break;
        case CursorType::IBeam: cursorStyle = "text"; break;
        case CursorType::Hand: cursorStyle = "pointer"; break;
        case CursorType::Crosshair: cursorStyle = "crosshair"; break;
        case CursorType::Wait: cursorStyle = "wait"; break;
        case CursorType::Help: cursorStyle = "help"; break;
        case CursorType::ResizeHorizontal: cursorStyle = "ew-resize"; break;
        case CursorType::ResizeVertical: cursorStyle = "ns-resize"; break;
        case CursorType::ResizeDiagonal: cursorStyle = "nwse-resize"; break;
    }
    
    EM_ASM({
        document.body.style.cursor = UTF8ToString($0);
    }, cursorStyle);
}

CursorType Mouse::GetCursorType() const {
    return g_currentCursorType;
}