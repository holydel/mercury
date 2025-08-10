#include "mercury_input.h"

using namespace mercury::input;

Keyboard *mercury::input::gKeyboard = nullptr;
Mouse *mercury::input::gMouse = nullptr;
Gyroscope *mercury::input::gGyroscope = nullptr;

Gamepad *mercury::input::gGamepads[MAX_GAMEPADS] = {nullptr};

XRController *mercury::input::gXRControllers[MAX_XR_CONTROLLERS] = {nullptr};

void MercuryInputInitialize() {
    // Initialize global input instances
    gKeyboard = new Keyboard();
    gMouse = new Mouse();
    gGyroscope = new Gyroscope();
    
    // Initialize gamepads
    for (int i = 0; i < MAX_GAMEPADS; i++) {
        gGamepads[i] = new Gamepad();
    }
    
    // Initialize XR controllers
    for (int i = 0; i < MAX_XR_CONTROLLERS; i++) {
        gXRControllers[i] = new XRController();
    }
}

void MercuryInputShutdown() {
    // Clean up global input instances
    delete gKeyboard;
    gKeyboard = nullptr;
    
    delete gMouse;
    gMouse = nullptr;
    
    delete gGyroscope;
    gGyroscope = nullptr;
    
    // Clean up gamepads
    for (int i = 0; i < MAX_GAMEPADS; i++) {
        delete gGamepads[i];
        gGamepads[i] = nullptr;
    }
    
    // Clean up XR controllers
    for (int i = 0; i < MAX_XR_CONTROLLERS; i++) {
        delete gXRControllers[i];
        gXRControllers[i] = nullptr;
    }
}