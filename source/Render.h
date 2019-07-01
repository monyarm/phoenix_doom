#include "Doom.h"

struct seg_t;
struct SpriteFrameAngle;
struct Texture;

//----------------------------------------------------------------------------------------------------------------------
// Renderer constants and limits
//----------------------------------------------------------------------------------------------------------------------

// Limits
static constexpr uint32_t   MAXVISSPRITES       = 128;                      // Maximum number of visible sprites
static constexpr uint32_t   MAXVISPLANES        = 64;                       // Maximum number of visible floor and ceiling textures
static constexpr uint32_t   MAXWALLCMDS         = 128;                      // Maximum number of visible walls
static constexpr uint32_t   MAXOPENINGS         = MAXSCREENWIDTH * 64;      // Space for sil tables
static constexpr Fixed      MINZ                = FRACUNIT * 4;             // Closest z allowed (clip sprites closer than this etc.)
static constexpr uint32_t   FIELDOFVIEW         = 2048;                     // 90 degrees of view

// Rendering constants
static constexpr uint32_t   HEIGHTBITS          = 6;                        // Number of bits for texture height
static constexpr uint32_t   FIXEDTOHEIGHT       = FRACBITS - HEIGHTBITS;    // Number of unused bits from fixed to SCALEBITS
static constexpr uint32_t   SCALEBITS           = 9;                        // Number of bits for texture scale
static constexpr uint32_t   FIXEDTOSCALE        = FRACBITS - SCALEBITS;     // Number of unused bits from fixed to HEIGHTBITS
static constexpr uint32_t   ANGLETOSKYSHIFT     = 22;                       // sky map is 256*128*4 maps

//----------------------------------------------------------------------------------------------------------------------
// Render data structures
//----------------------------------------------------------------------------------------------------------------------

// Describes a 2D shape rendered to the screen
struct vissprite_t {
    int32_t                     x1;             // Clipped to screen edges column range
    int32_t                     x2;
    int32_t                     y1;
    int32_t                     y2;
    Fixed                       xscale;         // Scale factor
    Fixed                       yscale;         // Y Scale factor
    const SpriteFrameAngle*     pSprite;        // What sprite frame to actually render
    uint32_t                    colormap;       // 0x8000 = shadow draw,0x4000 flip, 0x3FFF color map
    const mobj_t*               thing;          // Used for clipping...
};

// Describes a floor area to be drawn
struct visplane_t {
    uint32_t    open[MAXSCREENWIDTH + 1];   // top<<8 | bottom
    Fixed       height;                     // Height of the floor
    uint32_t    PicHandle;                  // Texture handle
    uint32_t    PlaneLight;                 // Light override
    int32_t     minx;                       // Minimum x, max x
    int32_t     maxx;
};

// Describe a wall segment to be drawn
struct viswall_t {        
    uint32_t    LeftX;          // Leftmost x screen coord
    uint32_t    RightX;         // Rightmost inclusive x coordinates
    uint32_t    FloorPic;       // Picture handle to floor shape
    uint32_t    CeilingPic;     // Picture handle to ceiling shape
    uint32_t    WallActions;    // Actions to perform for draw

    int32_t         t_topheight;        // Describe the top texture
    int32_t         t_bottomheight;
    int32_t         t_texturemid;
    const Texture*  t_texture;          // Pointer to the top texture
    
    int32_t         b_topheight;        // Describe the bottom texture
    int32_t         b_bottomheight;
    int32_t         b_texturemid;
    const Texture*  b_texture;          // Pointer to the bottom texture
    
    int32_t     floorheight;
    int32_t     floornewheight;
    int32_t     ceilingheight;
    int32_t     ceilingnewheight;

    Fixed       LeftScale;              // LeftX Scale
    Fixed       RightScale;             // RightX scale
    Fixed       SmallScale;
    Fixed       LargeScale;
    uint8_t*    TopSil;                 // YClips for the top line
    uint8_t*    BottomSil;              // YClips for the bottom line
    Fixed       ScaleStep;              // Scale step factor
    angle_t     CenterAngle;            // Center angle
    Fixed       offset;                 // Offset to the texture
    uint32_t    distance;
    uint32_t    seglightlevel;
    seg_t*      SegPtr;                 // Pointer to line segment for clipping  
};

// Wall 'action' flags for a vis wall
static constexpr uint32_t AC_ADDFLOOR       = 0x1;
static constexpr uint32_t AC_ADDCEILING     = 0x2;
static constexpr uint32_t AC_TOPTEXTURE     = 0x4;
static constexpr uint32_t AC_BOTTOMTEXTURE  = 0x8;
static constexpr uint32_t AC_NEWCEILING     = 0x10;
static constexpr uint32_t AC_NEWFLOOR       = 0x20;
static constexpr uint32_t AC_ADDSKY         = 0x40;
static constexpr uint32_t AC_TOPSIL         = 0x80;
static constexpr uint32_t AC_BOTTOMSIL      = 0x100;
static constexpr uint32_t AC_SOLIDSIL       = 0x200;

//----------------------------------------------------------------------------------------------------------------------
// Render data
//----------------------------------------------------------------------------------------------------------------------
void R_InitData();
void InitMathTables();

//----------------------------------------------------------------------------------------------------------------------
// Render main
//----------------------------------------------------------------------------------------------------------------------
extern viswall_t        viswalls[MAXWALLCMDS];          // Visible wall array
extern viswall_t*       lastwallcmd;                    // Pointer to free wall entry
extern visplane_t       visplanes[MAXVISPLANES];        // Visible floor array
extern visplane_t*      lastvisplane;                   // Pointer to free floor entry
extern vissprite_t      vissprites[MAXVISSPRITES];      // Visible sprite array
extern vissprite_t*     vissprite_p;                    // Pointer to free sprite entry
extern Byte             openings[MAXOPENINGS];
extern Byte*            lastopening;
extern Fixed            viewx;                          // Camera x,y,z
extern Fixed            viewy;
extern Fixed            viewz;
extern angle_t          viewangle;                      // Camera angle
extern Fixed            viewcos;                        // Camera sine, cosine from angle
extern Fixed            viewsin;
extern uint32_t         extralight;                     // Bumped light from gun blasts
extern angle_t          clipangle;                      // Leftmost clipping angle
extern angle_t          doubleclipangle;                // Doubled leftmost clipping angle

void R_Init() noexcept;
void R_Setup() noexcept;
void R_RenderPlayerView() noexcept;

//----------------------------------------------------------------------------------------------------------------------
// Render - 'phase 1'
//----------------------------------------------------------------------------------------------------------------------
extern uint32_t     SpriteTotal;        // Total number of sprites to render
extern uint32_t*    SortedSprites;      // Pointer to array of words of sprites to render

void BSP();

//----------------------------------------------------------------------------------------------------------------------
// Render - 'phase 2'
//----------------------------------------------------------------------------------------------------------------------
void WallPrep(uint32_t LeftX, uint32_t RightX, seg_t* LineSeg, angle_t LineAngle);

//----------------------------------------------------------------------------------------------------------------------
// Render - 'phase 6'
//----------------------------------------------------------------------------------------------------------------------
void SegCommands();

//----------------------------------------------------------------------------------------------------------------------
// Render - 'phase 7'
//----------------------------------------------------------------------------------------------------------------------
extern uint8_t*     PlaneSource;        // Pointer to floor shape
extern Fixed        planey;             // Latched viewx / viewy for floor drawing
extern Fixed        basexscale;
extern Fixed        baseyscale;

void DrawVisPlane(visplane_t* PlanePtr);

//----------------------------------------------------------------------------------------------------------------------
// Render - 'phase 8'
//----------------------------------------------------------------------------------------------------------------------
extern uint32_t spropening[MAXSCREENWIDTH];     // clipped range

uint32_t* SortWords(uint32_t* Before, uint32_t* After, uint32_t Total);
void DrawVisSprite(const vissprite_t* const pVisSprite);
void DrawWeapons();