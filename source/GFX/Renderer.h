#pragma once

#include "Base/Macros.h"

// Enable to allow moving up and down the camera along the Z axis.
// Useful to test how the renderer reacts to different height adjustments.
#ifndef ENABLE_DEBUG_CAMERA_Z_MOVEMENT
    #define ENABLE_DEBUG_CAMERA_Z_MOVEMENT 1
#endif

// TODO: REMOVE EVENTUALLY, once high resolution support is properly implemented.
// Define to enable a quick hack to test higher resolution rendering.
#define HACK_TEST_HIGH_RES_RENDERING 1
#define HACK_TEST_HIGH_RENDER_SCALE 3

struct CelImage;

BEGIN_NAMESPACE(Renderer)

#if ENABLE_DEBUG_CAMERA_Z_MOVEMENT
    extern float gDebugCameraZOffset;
#endif

// Used to compute scale factors
static constexpr uint32_t REFERENCE_SCREEN_WIDTH = 320;
static constexpr uint32_t REFERENCE_SCREEN_HEIGHT = 200;
static constexpr uint32_t REFERENCE_3D_VIEW_WIDTH = 280;
static constexpr uint32_t REFERENCE_3D_VIEW_HEIGHT = 160;

void init() noexcept;               // Initialize the renderer (done once)
void initMathTables() noexcept;     // Re-initialize the renderer math tables; must be done if screen size changes!
void drawPlayerView() noexcept;     // Render the 3d view for the player

// Utility function: draws a sprite which is scaled in accordance with the scale factor for the renderer.
// The coordinates are given in terms of the original 320x200 resolution.
void drawUISprite(const int32_t x, const int32_t y, const CelImage& image) noexcept;
void drawUISprite(const int32_t x, const int32_t y, const uint32_t resourceNum) noexcept;
void drawMaskedUISprite(const int32_t x, const int32_t y, const uint32_t resourceNum) noexcept;

END_NAMESPACE(Renderer)
