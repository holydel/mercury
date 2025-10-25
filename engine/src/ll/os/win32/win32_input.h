#pragma once
#include "mercury_input.h"
#include <windows.h>

// Input handling functions
void HandleKeyDown(WPARAM wParam, LPARAM lParam);
void HandleKeyUp(WPARAM wParam, LPARAM lParam);
void HandleChar(WPARAM wParam, LPARAM lParam);
void HandleMouseButton(UINT message, WPARAM wParam, LPARAM lParam, bool isDown);
void HandleMouseDoubleClick(UINT message, WPARAM wParam, LPARAM lParam);
void HandleMouseMove(WPARAM wParam, LPARAM lParam);

void RegisterWin32InputCallbacks();
void MercuryInputPreTick();
void MercuryInputPostTick();

extern glm::ivec2 gMousePosition;