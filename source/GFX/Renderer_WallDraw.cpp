#include "Renderer_Internal.h"

#include "Base/Tables.h"
#include "Blit.h"
#include "Map/Setup.h"
#include "Textures.h"
#include "Video.h"

//------------------------------------------------------------------------------------------------------------------------------------------
// Code for drawing walls and skies in the game.
//
// Notes:
//  (1) Clip values are the solid pixel bounding the range
//  (2) floorclip starts out ScreenHeight
//  (3) ceilingclip starts out -1
//------------------------------------------------------------------------------------------------------------------------------------------
BEGIN_NAMESPACE(Renderer)

//------------------------------------------------------------------------------------------------------------------------------------------
// Draws a single column of the sky
//------------------------------------------------------------------------------------------------------------------------------------------
static void drawSkyColumn(const uint32_t viewX, const uint32_t maxColHeight) noexcept {
    // Figure out the angle this sky column is at.
    // From that figure out the texture coordinate: the sky texture is 256 pixels wide and repeats 4 times over a circle.
    const angle_t angle = gViewAngleBAM + gScreenXToAngleBAM[viewX];
    const uint32_t texX = (angle >> 22) & 0xFFu;

    // Figure out the sky column height and texel step (y)
    const Texture* const pTex = (const Texture*) Textures::getWall(gSkyTextureNum);
    const ImageData& texImg = pTex->data;

    const uint32_t skyTexH = texImg.height;
    const Fixed skyScale = fixed16Div(intToFixed16((int32_t) g3dViewHeight), intToFixed16(Renderer::REFERENCE_3D_VIEW_HEIGHT));
    const Fixed scaledColHeight = fixed16Mul(intToFixed16((int32_t) skyTexH), skyScale);
    const uint32_t roundColHeight = ((scaledColHeight & FRACMASK) != 0) ? 1 : 0;
    const uint32_t colHeight = (uint32_t) fixed16ToInt(scaledColHeight) + roundColHeight;
    BLIT_ASSERT(colHeight < g3dViewHeight);

    const float texYStep = fixed16ToFloat(Blit::calcTexelStep(skyTexH, colHeight));

    // Draw the sky column
    Blit::blitColumn<
        Blit::BCF_STEP_Y
    >(
        texImg.pPixels,
        texImg.width,
        texImg.height,
        (float) texX,
        0.0f,
        0.0f,
        0.0f,
        Video::gpFrameBuffer + (uintptr_t) g3dViewYOffset * Video::gScreenWidth + g3dViewXOffset,
        g3dViewWidth,
        g3dViewHeight,
        Video::gScreenWidth,
        (int32_t) viewX,
        0,
        std::min(colHeight, maxColHeight),
        0,
        texYStep
    );
}

void drawAllWallFragments() noexcept {
    for (const WallFragment& wallFrag : gWallFragments) {
        const ImageData& wallImage = *wallFrag.pImageData;

        Blit::blitColumn<
            Blit::BCF_STEP_Y |
            Blit::BCF_H_WRAP_WRAP |
            Blit::BCF_V_WRAP_WRAP |
            Blit::BCF_COLOR_MULT_RGB
        >(
            wallImage.pPixels,
            wallImage.width,
            wallImage.height,
            (float) wallFrag.texcoordX,
            wallFrag.texcoordY,
            0.0f,
            wallFrag.texcoordYSubPixelAdjust,
            Video::gpFrameBuffer + (uintptr_t) g3dViewYOffset * Video::gScreenWidth + g3dViewXOffset,
            g3dViewWidth,
            g3dViewHeight,
            Video::gScreenWidth,
            wallFrag.x,
            wallFrag.y,
            wallFrag.height,
            0,
            wallFrag.texcoordYStep,
            wallFrag.lightMul,
            wallFrag.lightMul,
            wallFrag.lightMul
        );
    }
}

void drawAllSkyFragments() noexcept {
    for (const SkyFragment& skyFrag : gSkyFragments) {
        drawSkyColumn(skyFrag.x, skyFrag.height);
    }
}

END_NAMESPACE(Renderer)
