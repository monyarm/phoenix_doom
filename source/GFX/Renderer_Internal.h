#pragma once

//------------------------------------------------------------------------------------------------------------------------------------------
// Data structures and globals internal to the renderer.
// Nothing here is used by outside code.
//------------------------------------------------------------------------------------------------------------------------------------------
#include "Base/Angle.h"
#include "Game/DoomDefines.h"
#include "Renderer.h"
#include <cstddef>
#include <vector>

struct ImageData;
struct line_t;
struct mobj_t;
struct seg_t;
struct SpriteFrameAngle;
struct Texture;

namespace Renderer {
    //==================================================================================================================
    // Constants
    //==================================================================================================================

    // Limits
    static constexpr Fixed      MINZ                    = FRACUNIT * 4;             // Closest z allowed (clip sprites closer than this etc.)
    static constexpr uint32_t   FIELDOFVIEW             = 2048;                     // 90 degrees of view
    static constexpr float      MAX_LIGHT_VALUE         = 255.0f;
    static constexpr float      MIN_LIGHT_MUL           = 0.020f;                   // Minimum allowed multiplier due to light

    // Rendering constants
    static constexpr float      FOV                 = FMath::ANGLE_90<float>;       // Field of view for 3D perspective
    static constexpr float      Z_NEAR              = 1.0f;                         // Near plane Z value
    static constexpr float      Z_RANGE_SIZE        = 16384.0f;                     // Size of the range of Z values allowed
    static constexpr float      Z_FAR               = Z_NEAR + Z_RANGE_SIZE;        // Far plane Z value
    static constexpr float      VIEW_ASPECT_RATIO   = 2.0f;                         // Aspect ratio used for perspective projection

    // An extra Z offset applied to all rendered sprites: this is a strange one!
    //
    // The original 3DO Doom appeared to apply an offset of +5.0 to the Z position of all sprites to get them out of the ground.
    // I actually found this was 1 unit too and left a 1 pixel gap in many cases between the sprite and the ground, so I'm
    // adjusting back down to 4.0 for improved accuracy. As to WHY this offset is required, I have absolutely no idea...
    // Perhaps it is a consequence of how the sprites were processed for 3DO, or even something originating from the Jag port?
    //
    static constexpr float SPRITE_EXTRA_Z_OFFSET = 4.0f;

    //==================================================================================================================
    // Data structures
    //==================================================================================================================

    //------------------------------------------------------------------------------------------------------------------
    // Clip bounds (top & bottom) for when emitting seg columns (floor + wall)
    //------------------------------------------------------------------------------------------------------------------
    struct SegClip {
        int16_t top;
        int16_t bottom;
    };

    //------------------------------------------------------------------------------------------------------------------
    // Describes lighting params for an input light level
    //------------------------------------------------------------------------------------------------------------------
    struct LightParams {
        float   lightMin;       // Minimum light value allowed
        float   lightMax;       // Maximum light value allowed
        float   lightSub;       // Subtract this as part of the light diminishing calculations
        float   lightCoef;      // Controls the falloff for light diminishing

        // For these light parameters, gives a light multiplier that can be applied to textures etc.
        // after doing light diminishing effects. Requires the distance of the object from the camera.
        float getLightMulForDist(const float dist) const noexcept;
    };

    //------------------------------------------------------------------------------------------------------------------
    // Describes a sparse 4x4 matrix designed for 3D projections.
    //
    //  (1) 'Z' in this matrix is DEPTH (unlike the Doom coord sys). 'Y' is also height here.
    //  (2) The matrix (M) is designed transform a single row/vector (V) using the following multiply order: MxV.
    //  (3) Except where otherwise stated, omitted elements are '0' and are not considered in calculations.
    //  (4) The omitted 'r3c3' element is '1' and IS considered in calculations.
    //
    // Note that the calculations below are Loosely based on the GLM function 'perspectiveRH_ZO',
    // with various adjustments to make it match how the original 3DO Doom (and probably PC Doom) computed projections.
    //
    // Based on input parameters for view width & height, znear & far, and field of view respectively:
    //      w, h, zn, zf, fov
    //
    // The matrix is computed via the following method (note r3c3 is always omitted from storage):
    //      f = tan(fov * 0.5)
    //      a = w / h
    //
    //      r0c0 = 1 / f
    //      r1c1 = -a / f
    //      r2c2 = -zf / (zn - zf)
    //      r3c2 = -(zn * zf) / (zf - zn)
    //      r2c3 = 1.0
    //
    // Effectively it looks like this when visualized (with implicit entries added):
    //      r0c0,   0,      0,      0,
    //      0,      r1c1,   0,      0,
    //      0,      0,      r2c2,   r2c3,
    //      0,      0,      1.0,    0
    //------------------------------------------------------------------------------------------------------------------
    struct ProjectionMatrix {
        float r0c0;
        float r1c1;
        float r2c2;
        float r2c3;
    };

    //------------------------------------------------------------------------------------------------------------------
    // Describes 3D coordinates for line segment to be drawn.
    //
    // Note:
    //  (1) Coords are such that xy is the ground plane and z represents height. (Doom coord system)
    //  (2) Before perspective projection x and y give the 2d position on the map for the line segment.
    //  (3) After persective projection y is a 'depth' value into the screen.
    //  (4) 1/w represents the value scale at a point.
    //      It can also be used to help perform perspective correct interpolation.
    //------------------------------------------------------------------------------------------------------------------
    struct DrawSeg {
        float p1x;              // 1st wall point: xyw and 1/w (scale)
        float p1y;
        float p1w;
        float p1w_inv;

        float p2x;              // 2nd wall point: xyw and 1/w (scale)
        float p2y;
        float p2w;
        float p2w_inv;

        float p1tz;             // 1st wall point: top and bottom z
        float p1bz;
        float p1tz_back;        // 1st wall point: top and bottom z for the back/joining sector
        float p1bz_back;

        float p2tz;             // 2nd wall point: top and bottom z
        float p2bz;
        float p2tz_back;        // 2nd wall point: top and bottom z for the back/joining sector
        float p2bz_back;

        //--------------------------------------------------------------------------------------------------------------
        // Some flags on what to emit.
        // Set these early on during draw seg generation because it's easier to determine this stuff there!
        //--------------------------------------------------------------------------------------------------------------

        // Should we emit a floor or ceiling for a front facing seg?
        bool bEmitCeiling;
        bool bEmitFloor;

        // Set only for a two sided seg, undefined otherwise.
        // Whether we emit an upper or lower occluder for a seg.
        bool bEmitUpperOccluder;
        bool bEmitLowerOccluder;

        // Whether to use the front or back sector z values for upper and lower occluders.
        // This is determined based on the camera height relative to the occluding segs.
        // Unused for anything other than two sided segs.
        bool bUpperOccluderUsesBackZ;
        bool bLowerOccluderUsesBackZ;

        //--------------------------------------------------------------------------------------------------------------
        // Vertex attributes that are not affected by any transforms except for clipping.
        // These are interpolated across columns in the seg also.
        //--------------------------------------------------------------------------------------------------------------
        float p1TexX;           // X texture coordinate for p1 and p2
        float p2TexX;

        float p1WorldX;         // World x and y values for p1 and p2
        float p1WorldY;
        float p2WorldX;
        float p2WorldY;
    };

    //------------------------------------------------------------------------------------------------------------------
    // Contains details on a sprite that is queued to be drawn
    //------------------------------------------------------------------------------------------------------------------
    struct DrawSprite {
        const uint16_t*     pPixels;            // Pixels for the sprite (in column major format)
        float               worldX;             // World center X position of the sprite (used for occlusion tests)
        float               worldY;             // World center Y position of the sprite (used for occlusion tests)
        float               screenLx;           // Left and right screen X values
        float               screenRx;
        float               screenTy;           // Top and bottom screen Y values
        float               screenBy;
        float               depth;              // Depth of the sprite for the view (used for Z sorting)
        float               lightMul;           // Light multiplier
        uint16_t            texW;               // Width and height of sprite texture
        uint16_t            texH;
        bool                bFlip;              // Flip horizontally?
        bool                bTransparent;       // Used for the spectre demon
    };

    //------------------------------------------------------------------------------------------------------------------
    // Contains a fragment of a sprite to be drawn
    //------------------------------------------------------------------------------------------------------------------
    struct SpriteFragment {
        uint16_t            x;                      // Screen x and y
        int16_t             y;
        uint16_t            height;                 // Screen height
        uint16_t            texH : 15;              // Height of the sprite texture
        uint16_t            isTransparent : 1;      // If '1' then draw the sprite transparent (used for spectres)
        float               depth;                  // Depth that the sprite is at (used for all sprite fragments)
        float               lightMul;               // Light multiplier
        float               texYStep;               // Stepping to use for the 'Y' texture coordinate
        float               texYSubPixelAdjust;     // Sub-pixel adjustment for 'Y' texture coordinate. Applied to every pixel after the first.
        const uint16_t*     pSpriteColPixels;       // The image data for the sprite (in column major format)
        float               spriteWorldX;           // World center of the sprite: X
        float               spriteWorldY;           // World center of the sprite: Y
    };

    //------------------------------------------------------------------------------------------------------------------
    // Data structure that for every column on the screen describes all of the occluding columns.
    // Used for sprite rendering.
    //------------------------------------------------------------------------------------------------------------------
    struct OccludingColumns {
        static constexpr uint32_t MAX_ENTRIES = 16;

        // The number of occluding column entries.
        uint32_t count;

        // The depths of each occluding column.
        // Note: is automatically in ascending order due to the nature of the BSP tree rendering - hence can be binary searched!
        float depths[MAX_ENTRIES];

        // The lines for each occluding column.
        // Used for the purposes of sprite clipping.
        line_t* pLines[MAX_ENTRIES];

        // How much screen real estate each occluding column occupies at the top and bottom of the screen.
        // Anything at or above the top coordinate is occluded.
        // Anything at or below the bottom coordinate is occluded.
        struct Bounds {
            int16_t top;
            int16_t bottom;
        };

        Bounds bounds[MAX_ENTRIES];
    };

    //------------------------------------------------------------------------------------------------------------------
    // Describes a column of a wall to be drawn
    //------------------------------------------------------------------------------------------------------------------
    struct WallFragment {
        uint16_t            x;
        uint16_t            y;
        uint16_t            height;
        uint16_t            texcoordX;
        float               texcoordY;
        float               texcoordYSubPixelAdjust;    // Sub pixel stability adjustment applied after the first stepping
        float               texcoordYStep;
        float               lightMul;                   // Multiply value for lighting
        const ImageData*    pImageData;
    };

    //------------------------------------------------------------------------------------------------------------------
    // Describes a column of the sky to be drawn
    //------------------------------------------------------------------------------------------------------------------
    struct SkyFragment {
        uint16_t    x;
        uint16_t    height;
    };

    //------------------------------------------------------------------------------------------------------------------
    // Describes a column of a floor or ceiling to be drawn
    //------------------------------------------------------------------------------------------------------------------
    struct FlatFragment {
        uint16_t            x;
        uint16_t            y;
        uint16_t            height;
        uint8_t             sectorLightLevel;
        bool                bClampFirstPixel;       // If true clamp the texcoord of the first pixel in the column to prevent overstepping past the world position!
        float               depth;                  // Depth of the fragment in the view
        float               worldX;                 // World position at the wall which the column was generated at
        float               worldY;
        float               worldZ;
        const ImageData*    pImageData;
    };

    //==================================================================================================================
    // Globals shared throughout the renderer - defined in Renderer.cpp
    //==================================================================================================================

    extern Fixed                            gViewXFrac;                         // Camera x,y,z
    extern Fixed                            gViewYFrac;
    extern Fixed                            gViewZFrac;
    extern float                            gViewX;                             // Camera x,y,z
    extern float                            gViewY;
    extern float                            gViewZ;
    extern float                            gViewDirX;                          // View 2D forward/direction vector
    extern float                            gViewDirY;
    extern float                            gViewPerpX;                         // View 2D perpendicular/right vector
    extern float                            gViewPerpY;
    extern angle_t                          gViewAngleBAM;                      // Camera angle
    extern float                            gViewAngle;                         // Camera angle
    extern Fixed                            gViewCosFrac;                       // Camera sine, cosine from angle
    extern Fixed                            gViewSinFrac;
    extern float                            gViewCos;                           // Camera sine, cosine from angle
    extern float                            gViewSin;
    extern float                            gNearPlaneW;                        // Width and height of the near plane
    extern float                            gNearPlaneH;
    extern float                            gNearPlaneHalfW;                    // Half width and height of the near plane
    extern float                            gNearPlaneHalfH;
    extern float                            gNearPlaneP1x;                      // Near plane left side x,y
    extern float                            gNearPlaneP1y;
    extern float                            gNearPlaneP2x;                      // Near plane right side x,y
    extern float                            gNearPlaneP2y;
    extern float                            gNearPlaneTz;                       // Near plane top and bottom z
    extern float                            gNearPlaneBz;
    extern float                            gNearPlaneXStepPerViewCol;          // How much to step world x and y for each successive screen column of pixels at the near plane
    extern float                            gNearPlaneYStepPerViewCol;
    extern float                            gNearPlaneZStepPerViewColPixel;     // How much to step world z for each successive pixel in a screen column at the near plane
    extern ProjectionMatrix                 gProjMatrix;                        // 3D projection matrix
    extern uint32_t                         gExtraLight;                        // Bumped light from gun blasts
    extern std::vector<angle_t>             gScreenXToAngleBAM;                 // Convert from a screen X coordinate to a Doom format (BAM) angle
    extern std::vector<DrawSeg>             gDrawSegs;
    extern std::vector<SegClip>             gSegClip;                           // Used to clip seg columns (walls + floors) vertically as segs are being submitted. One entry per screen column.
    extern std::vector<OccludingColumns>    gOccludingCols;                     // Used to clip sprite columns. One entry per screen column.
    extern uint32_t                         gNumFullSegCols;                    // The number of columns that will accept no more seg pixels. Used to stop emitting segs when we have filled the screen.
    extern std::vector<WallFragment>        gWallFragments;                     // Wall fragments to be drawn
    extern std::vector<FlatFragment>        gFloorFragments;                    // Floor fragments to be drawn
    extern std::vector<FlatFragment>        gCeilFragments;                     // Ceiling fragments to be drawn
    extern std::vector<SkyFragment>         gSkyFragments;                      // Sky fragments to be drawn
    extern std::vector<DrawSprite>          gDrawSprites;                       // Sprites to be drawn that will later be turned into fragments (after depth sort)
    
    //==================================================================================================================
    // Functions
    //==================================================================================================================

    void doBspTraversal() noexcept;
    void addSegToFrame(seg_t& seg) noexcept;
    void addSpriteToFrame(const mobj_t& thing) noexcept;
    void drawAllLineSegs() noexcept;
    void drawAllWallFragments() noexcept;
    void drawAllFloorFragments() noexcept;
    void drawAllCeilingFragments() noexcept;
    void drawAllSkyFragments() noexcept;
    void drawAllSprites() noexcept;
    void drawWeapons() noexcept;
    void doPostFx() noexcept;

    // Get light parameters for a floor or wall at the given light level
    LightParams getLightParams(const uint32_t sectorLightLevel) noexcept;
}
