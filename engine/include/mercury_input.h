#pragma once
#include "mercury_api.h"
#include <glm/glm.hpp>

namespace mercury
{
  namespace input
  {

    enum class Key : u8
    {
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

    enum class MouseButton : u8
    {
      Left,
      Right,
      Middle,
      X1,
      X2,
    };

    enum class CursorType : u8
    {
      Arrow,
      IBeam,
      Hand,
      Crosshair,
      Wait,
      Help,
      ResizeHorizontal,
      ResizeVertical,
      ResizeDiagonal,
    };

    class Keyboard
    {
    public:
      Keyboard() = default;
      ~Keyboard() = default;

      bool IsKeyDown(Key key) const;
      bool IsKeyUp(Key key) const;
      bool IsKeyPressed(Key key) const;
      bool IsKeyReleased(Key key) const;
      c32 GetChar() const;
    };

    class Mouse
    {
    public:
      Mouse() = default;
      ~Mouse() = default;

      bool IsButtonDown(MouseButton button) const;
      bool IsButtonUp(MouseButton button) const;
      bool IsButtonPressed(MouseButton button) const;
      bool IsButtonReleased(MouseButton button) const;

      bool IsButtonDoubleClicked(MouseButton button) const;

      void SetCursorType(CursorType type) const;
      CursorType GetCursorType() const;
    };

    class Gamepad
    {
    public:
      struct State
      {
        glm::vec2 leftStick;
        glm::vec2 rightStick;
        float leftTrigger;
        float rightTrigger;
        bool A, B, X, Y, LB, RB, Back, Start, LS, RS, Up, Down, Left, Right;
      };

    private:
      State currentState;

    public:
      Gamepad() = default;
      ~Gamepad() = default;

      const State &GetState() const
      {
        return currentState;
      }
      float GetBatteryLevel() const;
      bool IsConnected() const;
      // bool IsButtonDown(GamepadButton button);
    };

    class Gyroscope
    {
    public:
      Gyroscope() = default;
      ~Gyroscope() = default;
    };

    class XRController
    {
    public:
      XRController() = default;
      ~XRController() = default;
    };

    extern Keyboard *gKeyboard;
    extern Mouse *gMouse;
    extern Gyroscope *gGyroscope;

    constexpr u8 MAX_GAMEPADS = 4;
    extern Gamepad *gGamepads[MAX_GAMEPADS];

    constexpr u8 MAX_XR_CONTROLLERS = 2;
    extern XRController *gXRControllers[MAX_XR_CONTROLLERS];
  } // namespace input
} // namespace mercury
