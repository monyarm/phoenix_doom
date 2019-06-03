#include "MapData.h"

#include "doomrez.h"
#include "Endian.h"
#include "Macros.h"
#include "Resources.h"
#include <vector>

// On-disk versions of various map data structures.
// These differ in many cases to the runtime versions.
struct MapSector {
    Fixed       floorHeight;
    Fixed       ceilingHeight;
    uint32_t    floorPic;           // Floor flat number
    uint32_t    ceilingPic;         // Ceiling flat number
    uint32_t    lightLevel;
    uint32_t    special;            // Special flags
    uint32_t    tag;                // Tag ID
};

struct MapSide {
    Fixed       textureOffset;
    Fixed       rowOffset;
    uint32_t    topTexture;
    uint32_t    bottomTexture;
    uint32_t    midTexture;
    uint32_t    sector;
};

// Order of the lumps in a Doom wad (and also the 3DO resource file)
enum {
    ML_THINGS,
    ML_LINEDEFS,
    ML_SIDEDEFS,
    ML_VERTEXES,
    ML_SEGS,
    ML_SSECTORS,
    ML_SECTORS,
    ML_NODES,
    ML_REJECT,
    ML_BLOCKMAP,
    ML_TOTAL
};

// Internal vertex arrays
static std::vector<vertex_t>    gVertexes;
static std::vector<sector_t>    gSectors;
static std::vector<side_t>      gSides;

static void loadVertexes(const uint32_t lumpResourceNum) noexcept {
    const Resource* const pResource = loadResource(lumpResourceNum);
    
    const uint32_t numVerts = pResource->size / sizeof(vertex_t);
    gNumVertexes = numVerts;
    gVertexes.resize(numVerts);
    gpVertexes = gVertexes.data();
    
    const vertex_t* pSrcVert = (const vertex_t*) pResource->pData;
    const vertex_t* const pEndSrcVerts = (const vertex_t*) pResource->pData + numVerts;
    vertex_t* pDstVert = gVertexes.data();
    
    while (pSrcVert < pEndSrcVerts) {
        pDstVert->x = byteSwappedI32(pSrcVert->x);
        pDstVert->y = byteSwappedI32(pSrcVert->y);
        ++pSrcVert;
        ++pDstVert;
    }
    
    freeResource(lumpResourceNum);
}

static void loadSectors(const uint32_t lumpResourceNum) noexcept {
    // Load the sectors resource
    const Resource* const pResource = loadResource(lumpResourceNum);
    const std::byte* const pResourceData = (const std::byte*) pResource->pData;
    
    // Get the number of sectors first (first u32)
    const uint32_t numSectors = byteSwappedU32(((const uint32_t*) pResourceData)[0]);
    gNumSectors = numSectors;
    
    // Get the source sector data and alloc room for the runtime sectors
    const MapSector* pSrcSector = (const MapSector*)(pResourceData + sizeof(uint32_t));
    const MapSector* const pEndSrcSectors = pSrcSector + numSectors;
    
    gSectors.clear();
    gSectors.resize(numSectors);
    gpSectors = gSectors.data();
    
    // Create all sectors
    sector_t* pDstSector = gSectors.data();
    
    while (pSrcSector < pEndSrcSectors) {
        pDstSector->floorheight = byteSwappedI32(pSrcSector->floorHeight);
        pDstSector->ceilingheight = byteSwappedI32(pSrcSector->ceilingHeight);
        pDstSector->FloorPic = byteSwappedU32(pSrcSector->floorPic);
        pDstSector->CeilingPic = byteSwappedU32(pSrcSector->ceilingPic);
        pDstSector->lightlevel = byteSwappedU32(pSrcSector->lightLevel);
        pDstSector->special = byteSwappedU32(pSrcSector->special);
        pDstSector->tag  = byteSwappedU32(pSrcSector->tag);
        
        ++pSrcSector;
        ++pDstSector;
    }
    
    // Don't need this anymore
    freeResource(lumpResourceNum);
}

static void loadSides(const uint32_t lumpResourceNum) noexcept {
    // Load the side defs resource
    ASSERT_LOG(gSectors.size() > 0, "Sectors must be loaded first!");
    const Resource* const pResource = loadResource(lumpResourceNum);
    const std::byte* const pResourceData = (const std::byte*) pResource->pData;
    
    // Get the number of side defs first (first u32)
    const uint32_t numSides = byteSwappedU32(((const uint32_t*) pResourceData)[0]);
    gNumSides = numSides;
    
    // Get the source side def data and alloc room for the runtime side defs
    const MapSide* pSrcSide = (const MapSide*)(pResourceData + sizeof(uint32_t));
    const MapSide* const pEndSrcSides = pSrcSide + numSides;
    
    gSides.clear();
    gSides.resize(numSides);
    gpSides = gSides.data();
    
    // Create all sides
    side_t* pDstSide = gSides.data();
    
    while (pSrcSide < pEndSrcSides) {
        pDstSide->textureoffset = byteSwappedI32(pSrcSide->textureOffset);
        pDstSide->rowoffset = byteSwappedI32(pSrcSide->rowOffset);
        pDstSide->toptexture = byteSwappedU32(pSrcSide->topTexture);
        pDstSide->bottomtexture = byteSwappedU32(pSrcSide->bottomTexture);
        pDstSide->midtexture = byteSwappedU32(pSrcSide->midTexture);
        
        const uint32_t sectorNum = byteSwappedU32(pSrcSide->sector);
        pDstSide->sector = &gpSectors[sectorNum];
        
        ++pSrcSide;
        ++pDstSide;
    }
    
    // Don't need this anymore
    freeResource(lumpResourceNum);
}

#ifdef __cplusplus
extern "C" {
#endif

// External data pointers and information
const vertex_t*     gpVertexes;
uint32_t            gNumVertexes;
sector_t*           gpSectors;
uint32_t            gNumSectors;
side_t*             gpSides;
uint32_t            gNumSides;

void mapDataInit(const uint32_t mapNum) {
    // Figure out the resource number for the first map lump
    const uint32_t mapStartLump = ((mapNum - 1) * ML_TOTAL) + rMAP01;
    
    // Load all the map data.
    // N.B: must be done in this order due to data dependencies!
    loadVertexes(mapStartLump + ML_VERTEXES);
    loadSectors(mapStartLump + ML_SECTORS);
    loadSides(mapStartLump + ML_SIDEDEFS);
}

void mapDataShutdown() {
    gVertexes.clear();
    gpVertexes = nullptr;
    gNumVertexes = 0;
    
    gSectors.clear();
    gpSectors = nullptr;
    gNumSectors = 0;
    
    gSides.clear();
    gpSides = nullptr;
    gNumSides = 0;
}

#ifdef __cplusplus
}
#endif
