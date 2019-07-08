#include "Textures.h"

#include "Base/Endian.h"
#include "Base/Macros.h"
#include "Game/Data.h"
#include "Game/DoomRez.h"
#include "Game/Resources.h"
#include <vector>

struct TextureInfoHeader {
    uint32_t    numWallTextures;
    uint32_t    firstWallTexture;       // Resource number
    uint32_t    numFlatTextures;
    uint32_t    firstFlatTexture;       // Resource number
    
    void swapEndian() noexcept {
        byteSwapU32(numWallTextures);
        byteSwapU32(firstWallTexture);
        byteSwapU32(numFlatTextures);
        byteSwapU32(firstFlatTexture);
    }
};

struct TextureInfoEntry {
    uint32_t    width;
    uint32_t    height;
    uint32_t    _unused;
    
    void swapEndian() noexcept {
        byteSwapU32(width);
        byteSwapU32(height);
    }
};

static uint32_t                 gFirstWallTexResourceNum;
static uint32_t                 gFirstFlatTexResourceNum;
static std::vector<Texture>     gWallTextures;
static std::vector<Texture>     gFlatTextures;

static void loadTexture(Texture& tex, uint32_t textureNum) noexcept {    
    tex.pData = loadResourceData(tex.resourceNum);
    tex.animTexNum = textureNum;    // Initially redirects to itself for animation
}

static void freeTexture(Texture& tex) noexcept {
    if (tex.pData) {
        freeResource(tex.resourceNum);
        tex.pData = nullptr;
    }
}

static void freeTextures(std::vector<Texture>& textures) noexcept {
    for (Texture& texture : textures) {
        freeTexture(texture);
    }
}

static void clearTextures(std::vector<Texture>& textures) noexcept {
    freeTextures(textures);
    textures.clear();
}

void texturesInit() {
    // Read the header for all the texture info.
    // Note that we do NOT byte swap the original resources the may be cached and reused multiple times.
    // If we byte swapped the originals then we might double swap back to big endian accidently...
    const std::byte* pData = (const std::byte*) loadResourceData(rTEXTURE1);
    
    TextureInfoHeader header = (const TextureInfoHeader&) *pData;
    header.swapEndian();
    pData += sizeof(TextureInfoHeader);
    
    // Save global texture info and alloc memory for all the texture entries
    gFirstWallTexResourceNum = header.firstWallTexture;
    gFirstFlatTexResourceNum = header.firstFlatTexture;
    gWallTextures.resize(header.numWallTextures);
    gFlatTextures.resize(header.numFlatTextures);
    
    // Read the texture info for each wall texture
    {
        const uint32_t numWallTex = (uint32_t) gWallTextures.size();
        
        for (uint32_t wallTexNum = 0; wallTexNum < numWallTex; ++wallTexNum) {
            TextureInfoEntry info = (const TextureInfoEntry&) *pData;
            info.swapEndian();
            pData += sizeof(TextureInfoEntry);
            
            Texture& texture = gWallTextures[wallTexNum];
            texture.width = info.width;
            texture.height = info.height;
            texture.resourceNum = header.firstWallTexture + wallTexNum;
            texture.animTexNum = wallTexNum;    // Points to itself initallly (no anim)
        }
    }
    
    // Now done with this resource
    releaseResource(rTEXTURE1);
    
    // We don't have texture info for flats, all flats for 3DO are 64x64.
    // This was done orignally to help optimize the flat renderer, which was done in software on the 3DO's CPU.
    {
        const uint32_t numFlatTex = (uint32_t) gFlatTextures.size();
        
        for (uint32_t flatTexNum = 0; flatTexNum < numFlatTex; ++flatTexNum) {
            Texture& texture = gFlatTextures[flatTexNum];
            texture.width = 64;
            texture.height = 64;
            texture.resourceNum = header.firstFlatTexture + flatTexNum;
        }
    }
}
    
void texturesShutdown() {
    clearTextures(gWallTextures);
    clearTextures(gFlatTextures);
    gFirstWallTexResourceNum = 0;
    gFirstFlatTexResourceNum = 0;
}

void texturesFreeAll() {
    freeTextures(gWallTextures);
    freeTextures(gFlatTextures);
}

uint32_t getNumWallTextures() {
    return (uint32_t) gWallTextures.size();
}

uint32_t getNumFlatTextures() {
    return (uint32_t) gFlatTextures.size();
}

uint32_t getSky1TexNum() {
    return (uint32_t) rSKY1 - gFirstWallTexResourceNum;
}

uint32_t getSky2TexNum() {
    return (uint32_t) rSKY2 - gFirstWallTexResourceNum;
}

uint32_t getSky3TexNum() {
    return (uint32_t) rSKY3 - gFirstWallTexResourceNum;
}

uint32_t getCurrentSkyTexNum() {
    if (gGameMap < 9 || gGameMap == 24) {
        return getSky1TexNum();
    } else if (gGameMap < 18) {
        return getSky2TexNum();
    } else {
        return getSky3TexNum();
    }
}

const Texture* getWallTexture(const uint32_t num) {
    ASSERT(num < gWallTextures.size());
    return &gWallTextures[num];
}

const Texture* getFlatTexture(const uint32_t num) {
    ASSERT(num < gFlatTextures.size());
    return &gFlatTextures[num];
}

void loadWallTexture(const uint32_t num) {
    ASSERT(num < gWallTextures.size());
    loadTexture(gWallTextures[num], num);
}

void loadFlatTexture(const uint32_t num) {
    ASSERT(num < gFlatTextures.size());
    loadTexture(gFlatTextures[num], num);
}

void freeWallTexture(const uint32_t num) {
    ASSERT(num < gWallTextures.size());
    freeTexture(gWallTextures[num]);
}

void freeFlatTexture(const uint32_t num) {
    ASSERT(num < gFlatTextures.size());
    freeTexture(gFlatTextures[num]);
}

void setWallAnimTexNum(const uint32_t num, const uint32_t animTexNum) {
    ASSERT(num < gWallTextures.size());
    ASSERT(animTexNum < gWallTextures.size());
    gWallTextures[num].animTexNum = animTexNum;
}

void setFlatAnimTexNum(const uint32_t num, const uint32_t animTexNum) {
    ASSERT(num < gFlatTextures.size());
    ASSERT(animTexNum < gFlatTextures.size());
    gFlatTextures[num].animTexNum = animTexNum;
}

const Texture* getWallAnimTexture(const uint32_t num) {
    const Texture* const pOrigTexture = getWallTexture(num);
    return getWallTexture(pOrigTexture->animTexNum);
}

const Texture* getFlatAnimTexture(const uint32_t num) {
    const Texture* const pOrigTexture = getFlatTexture(num);
    return getFlatTexture(pOrigTexture->animTexNum);
}