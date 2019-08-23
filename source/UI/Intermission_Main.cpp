#include "Intermission_Main.h"

#include "Audio/Sound.h"
#include "Audio/Sounds.h"
#include "Burger.h"
#include "Game/Data.h"
#include "Game/DoomDefines.h"
#include "Game/DoomRez.h"
#include "Game/Resources.h"
#include "GFX/CelImages.h"
#include "GFX/Video.h"
#include <cstring>

#define KVALX   232
#define KVALY   70
#define IVALX   232
#define IVALY   100
#define SVALX   231
#define SVALY   130
#define INTERTIME (TICKSPERSEC/30)

enum {      // Intermission shape group
    KillShape,
    ItemsShape,
    SecretsShape
};

/**********************************

    Here are the names of each level

**********************************/

static constexpr const char* const MAP_NAMES[] = {
    "Hangar",
    "Nuclear Plant",
    "Toxin Refinery",
    "Command Control",
    "Phobos Lab",
    "Central Processing",
    "Computer Station",
    "Phobos Anomaly",
    "Deimos Anomaly",
    "Containment Area",
    "Refinery",
    "Deimos Lab",
    "Command Center",
    "Halls of the Damned",
    "Spawning Vats",
    "Tower of Babel",
    "Hell Keep",
    "Pandemonium",
    "House of Pain",
    "Unholy Cathedral",
    "Mt. Erebus",
    "Limbo",
    "Dis",
    "Military Base"
};

static constexpr const char* const FINISHED = "Finished";
static constexpr const char* const ENTERING = "Entering";

static uint32_t gKillPercent;        // Percent to attain
static uint32_t gItemPercent;
static uint32_t gSecretPercent;
static uint32_t gKillValue;         // Displayed percent value
static uint32_t gItemValue;
static uint32_t gSecretValue;
static uint32_t gINDelay;            // Delay before next inc
static uint32_t gBangCount;          // Delay for gunshot sound

/**********************************

    Print a string using the large font.
    I only load the large ASCII font if it is needed.

**********************************/
void PrintBigFont(int32_t x, int32_t y, const char* string) noexcept {
    uint32_t y2,c;
    const CelImageArray* gpUCharx;
    const CelImageArray* gpCurrent;

    c = string[0];      // Get the first char
    if (!c) {           // No string to print?
        return;         // Exit now
    }

    gpUCharx = nullptr;     // Assume ASCII font is NOT loaded

    do {
        ++string;       // Place here so "continue" will work
        y2 = y;         // Assume normal y coord
        gpCurrent = gpBigNumFont;   // Assume numeric font
        if (c >= '0' && c<='9') {
            c-= '0';
        } else if (c=='%') {        // Percent
            c = 10;
        } else if (c=='-') {        // Minus
            c = 11;
        } else {
            gpCurrent = gpUCharx;   // Assume I use the ASCII set
            if (c >= 'A' && c <= 'Z') { // Upper case?
                c-='A';
            } else if (c >= 'a' && c <= 'z') {
                c -= ('a'- 26);     // Index to lower case text
                y2 += 3;
            } else if (c=='.') {        // Period
                c = 52;
                y2 += 3;
            } else if (c=='!') {    // Exclaimation point
                c = 53;
                y2 += 3;
            } else {        // Hmmm, not supported!
                x += 6;       // Fake space
                continue;
            }
        }

        // Do I need the ASCII set? Make sure I have the text font.
        if (!gpCurrent) {
            gpUCharx = &CelImages::loadImages(rCHARSET, CelLoadFlagBits::MASKED);
            gpCurrent = gpUCharx;
        }

        const CelImage& shape = gpCurrent->getImage(c);     // Get the shape pointer
        Renderer::drawUISprite(x, y2, shape);               // Draw the char
        x += shape.width + 1;                               // Get the width to tab
    } while ((c = string[0])!=0);                           // Next index

    if (gpUCharx) {                             // Did I load the ASCII font?
        CelImages::releaseImages(rCHARSET);     // Release the ASCII font
    }
}

/**********************************

    Return the width of an ASCII string in pixels
    using the large font.

**********************************/

uint32_t GetBigStringWidth(const char* string) noexcept {
    uint32_t c,Width;
    const CelImageArray* gpUCharx;
    const CelImageArray* gpCurrent;

    c = string[0];      // Get a char
    if (!c) {           // No string to print?
        return 0;
    }
    gpUCharx = nullptr; // Only load in the ASCII set if I really need it
    Width = 0;
    do {
        ++string;
        gpCurrent = gpBigNumFont;   // Assume numeric font
        if (c >= '0' && c<='9') {
            c-= '0';
        } else if (c=='%') {        // Percent
            c = 10;
        } else if (c=='-') {        // Minus
            c = 11;
        } else {
            gpCurrent = gpUCharx;   // Assume I use the ASCII set
            if (c >= 'A' && c <= 'Z') { // Upper case?
                c-='A';
            } else if (c >= 'a' && c <= 'z') {
                c -= ('a'-26);      // Index to lower case text
            } else if (c=='.') {        // Period
                c = 52;
            } else if (c=='!') {    // Exclaimation point
                c = 53;
            } else {        // Hmmm, not supported!
                Width+=6;       // Fake space
                continue;
            }
        }

        // Do I need the ASCII set? Make sure I have the text font.
        if (!gpCurrent) {
            gpUCharx = &CelImages::loadImages(rCHARSET, CelLoadFlagBits::MASKED);
            gpCurrent = gpUCharx;
        }

        const CelImage& shape = gpCurrent->getImage(c);     // Get the shape pointer
        Width += shape.width + 1;                           // Get the width to tab
    } while ((c = string[0])!=0);                           // Next index

    if (gpUCharx) {                             // Did I load the ASCII font?
        CelImages::releaseImages(rCHARSET);     // Release the ASCII font
    }

    return Width;
}

//----------------------------------------------------------------------------------------------------------------------
// Draws a number, this number may be appended with a percent sign and or centered upon the x coord.
// I use flags PNPercent and PNCenter.
//----------------------------------------------------------------------------------------------------------------------
void PrintNumber(int32_t x, int32_t y, uint32_t value, uint32_t Flags) noexcept {
    char v[16];                                     // Buffer for text string
    LongWordToAscii(value, v);                      // Convert to string
    value = (uint32_t) std::strlen((char*) v);      // Get the length in chars

    if (Flags& PNFLAGS_PERCENT) {                   // Append a percent sign?
        v[value] = '%';                             // Append it
        ++value;
        v[value] = 0;                               // Make sure it's zero terminated
    }

    if (Flags & PNFLAGS_CENTER) {       // Center it?
        PrintBigFontCenter(x, y, v);
        return;
    }

    if (Flags & PNFLAGS_RIGHT) {        // Right justified?
        x -= GetBigStringWidth(v);
    }

    PrintBigFont(x, y, v);  // Print the string
}

//----------------------------------------------------------------------------------------------------------------------
// Print an ascii string centered on the x coord
//----------------------------------------------------------------------------------------------------------------------
void PrintBigFontCenter(const int32_t x, const int32_t y, const char* const str) noexcept {
    const int32_t w = (GetBigStringWidth(str) / 2);
    PrintBigFont(x - w, y, str);
}

/**********************************

    Init the intermission data

**********************************/
void IN_Start() noexcept {
    gINDelay = 0;
    gBangCount = 0;
    gKillValue = gItemValue = gSecretValue = 0;    // All values shown are zero
    gKillPercent = gItemPercent = gSecretPercent = 100;    // Init in case of divide by zero
    if (gTotalKillsInLevel) {            // Prevent divide by zeros
        gKillPercent = (gPlayer.killcount * 100) / gTotalKillsInLevel;
    }
    if (gItemsFoundInLevel) {
        gItemPercent = (gPlayer.itemcount * 100) / gItemsFoundInLevel;
    }
    if (gSecretsFoundInLevel) {
        gSecretPercent = (gPlayer.secretcount * 100) / gSecretsFoundInLevel;
    }
    S_StartSong(Song_intermission);     // Begin the music
}

/**********************************

    Exit the intermission

**********************************/
void IN_Stop() noexcept {
    S_StopSong();       // Kill the music
}

/**********************************

    Exit the intermission

**********************************/
gameaction_e IN_Ticker() noexcept {
    uint32_t Bang;
    if (gTotalGameTicks < (TICKSPERSEC/2)) { // Initial wait before I begin
        return ga_nothing;      // Do nothing
    }

    if (gNewJoyPadButtons & (PadA|PadB|PadC)) {  // Pressed a button?
        gKillValue = gKillPercent;        // Set to maximums
        gItemValue = gItemPercent;
        gSecretValue = gSecretPercent;
        return ga_died;     // Exit after drawing
    }

    ++gINDelay;

    if (gINDelay>=INTERTIME) {
        Bang = false;
        gINDelay-=INTERTIME;
        if (gKillValue < gKillPercent) {      // Is it too high?
            ++gKillValue;
            Bang = true;
        }
        if (gItemValue < gItemPercent) {
            ++gItemValue;
            Bang = true;
        }
        if (gSecretValue < gSecretPercent) {
            ++gSecretValue;
            Bang = true;
        }
        if (Bang) {
            ++gBangCount;
            if (!(gBangCount&3)) {
                S_StartSound(0,sfx_pistol);
            }
        }
    }
    return ga_nothing;      // Still here!
}

/**********************************

    Draw the intermission screen

**********************************/
void IN_Drawer(const bool bPresent, const bool bSaveFrameBuffer) noexcept {
    Video::debugClear();
    Renderer::drawUISprite(0, 0, rBACKGRNDBROWN);   // Load and draw the skulls

    const CelImageArray& intermisShapes = CelImages::loadImages(rINTERMIS, CelLoadFlagBits::MASKED);

    PrintBigFontCenter(160,10, MAP_NAMES[gGameMap-1]);              // Print the current map name
    PrintBigFontCenter(160,34, FINISHED);                           // Print "Finished"

    if (gNextMap != 23) {
        PrintBigFontCenter(160,162, ENTERING);
        PrintBigFontCenter(160,182, MAP_NAMES[gNextMap-1]);
    }

    Renderer::drawUISprite(71, KVALY, intermisShapes.getImage(KillShape));   // Draw the shapes
    Renderer::drawUISprite(65, IVALY, intermisShapes.getImage(ItemsShape));
    Renderer::drawUISprite(27, SVALY, intermisShapes.getImage(SecretsShape));

    PrintNumber(KVALX, KVALY, gKillValue, PNFLAGS_PERCENT|PNFLAGS_RIGHT);   // Print the numbers
    PrintNumber(IVALX, IVALY, gItemValue, PNFLAGS_PERCENT|PNFLAGS_RIGHT);
    PrintNumber(SVALX, SVALY, gSecretValue, PNFLAGS_PERCENT|PNFLAGS_RIGHT);

    CelImages::releaseImages(rINTERMIS);
    Video::endFrame(bPresent, bSaveFrameBuffer);
}
