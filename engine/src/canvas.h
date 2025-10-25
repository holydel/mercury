#pragma once
#include <mercury_canvas.h>
#include <ll/graphics.h>

void MercuryCanvasInitialize(int numFramesInFlight);
void MercuryCanvasShutdown();
void MercuryCanvasTick(mercury::ll::graphics::CommandList& cl, int frameInFlightIndex);

struct Scene2DConstants
{
    glm::mat2x2 prerptationMatrix;
    glm::vec4 canvasSize;

    float time;
    float deltaTime;
    float padding[2];
};