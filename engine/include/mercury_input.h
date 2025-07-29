#pragma once
#include "mercury_api.h"

namespace mercury {
namespace input {

enum class Key : u8 {
  A,
  B,
  C,
  D,
  E,
  F,
  G,
  H,
  I,
  J,
  K,
  L,
  M,
  N,
  O,
  P,
  Q,
  R,
  S,
  T,
  U,
  V,
  W,
  X,
  Y,
  Z,
  Num0,
  Num1,
  Num2,
  Num3,
  Num4,
  Num5,
  Num6,
  Num7,
  Num8,
  Num9,
  F1,
  F2,
  F3,
  F4,
  F5,
  F6,
  F7,
  F8,
  F9,
  F10,
  F11,
  F12,
  Escape,
  Tab,
  Shift,
  Control,
  Alt,
  Super,
  Space,
  Enter,
  Backspace,
  Delete,
};

enum class MouseButton : u8 {
  Left,
  Right,
  Middle,
  X1,
  X2,
};

class Keyboard {
public:
  Keyboard();
  ~Keyboard();

  void Initialize();
  void Shutdown();

  bool IsKeyDown(Key key);
  bool IsKeyUp(Key key);
  bool IsKeyPressed(Key key);
  bool IsKeyReleased(Key key);
};

class Mouse {
public:
  Mouse();

  void Initialize();
  void Shutdown();

  bool IsButtonDown(MouseButton button);
  bool IsButtonUp(MouseButton button);
  bool IsButtonPressed(MouseButton button);
  bool IsButtonReleased(MouseButton button);

  bool IsButtonDoubleClicked(MouseButton button);
};

class Gamepad {
public:
  Gamepad();
  ~Gamepad();

  void Initialize();
  void Shutdown();

  // bool IsButtonDown(GamepadButton button);
};

extern Keyboard *gKeyboard;
extern Mouse *gMouse;

constexpr u8 MAX_GAMEPADS = 4;
extern Gamepad *gGamepads[MAX_GAMEPADS];
} // namespace input
} // namespace mercury
