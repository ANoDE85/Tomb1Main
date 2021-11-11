#include "game/memory.h"

#include "specific/init.h"

#include <stdlib.h>
#include <string.h>

#define MALLOC_SIZE 0x1000000 // 16 MB

static char *m_GameMemoryPointer = NULL;
static char *m_GameAllocMemPointer = NULL;
static uint32_t m_GameAllocMemFree = 0;

static const char *m_BufferNames[] = {
    "Sprite Textures", // GBUF_TEXTURE_PAGES
    "Object Textures", // GBUF_OBJECT_TEXTURES
    "Mesh Pointers", // GBUF_MESH_POINTERS
    "Meshes", // GBUF_MESHES
    "Anims", // GBUF_ANIMS
    "Structs", // GBUF_ANIM_CHANGES
    "Ranges", // GBUF_ANIM_RANGES
    "Commands", // GBUF_ANIM_COMMANDS
    "Bones", // GBUF_ANIM_BONES
    "Frames", // GBUF_ANIM_FRAMES
    "Room Textures", // GBUF_ROOM_TEXTURES
    "Room Infos", // GBUF_ROOM_INFOS
    "Room Mesh", // GBUF_ROOM_MESH
    "Room Door", // GBUF_ROOM_DOOR
    "Room Floor", // GBUF_ROOM_FLOOR
    "Room Lights", // GBUF_ROOM_LIGHTS
    "Room Static Mesh Infos", // GBUF_ROOM_STATIC_MESH_INFOS
    "Floor Data", // GBUF_FLOOR_DATA
    "ITEMS!!", // GBUF_ITEMS
    "Cameras", // GBUF_CAMERAS
    "Sound FX", // GBUF_SOUND_FX
    "Boxes", // GBUF_BOXES
    "Overlaps", // GBUF_OVERLAPS
    "GroundZone", // GBUF_GROUNDZONE
    "FlyZone", // GBUF_FLYZONE
    "Animating Texture Ranges", // GBUF_ANIMATING_TEXTURE_RANGES
    "Cinematic Frames", // GBUF_CINEMATIC_FRAMES
    "LoadDemo Buffer", // GBUF_LOADDEMO_BUFFER
    "SaveDemo Buffer", // GBUF_SAVEDEMO_BUFFER
    "Cinematic Effects", // GBUF_CINEMATIC_EFFECTS
    "Mummy Head Turn", // GBUF_MUMMY_HEAD_TURN
    "Extra Door stuff", // GBUF_EXTRA_DOOR_STUFF
    "Effects_Array", // GBUF_EFFECTS
    "Creature Data", // GBUF_CREATURE_DATA
    "Creature LOT", // GBUF_CREATURE_LOT
    "Sample Infos", // GBUF_SAMPLE_INFOS
    "Samples", // GBUF_SAMPLES
    "Sample Offsets", // GBUF_SAMPLE_OFFSETS
    "Rolling Ball Stuff", // GBUF_ROLLINGBALL_STUFF
};

void Memory_Init()
{
    m_GameMemoryPointer = malloc(MALLOC_SIZE);
    if (!m_GameMemoryPointer) {
        S_ExitSystem("ERROR: Could not allocate enough memory");
    }
    memset(m_GameMemoryPointer, 0, MALLOC_SIZE);

    m_GameAllocMemPointer = m_GameMemoryPointer;
    m_GameAllocMemFree = MALLOC_SIZE;
}

void Memory_Shutdown()
{
    if (m_GameMemoryPointer) {
        free(m_GameMemoryPointer);
    }
    m_GameMemoryPointer = NULL;
    m_GameAllocMemPointer = NULL;
    m_GameAllocMemFree = 0;
}

void *Memory_Alloc(int32_t alloc_size, GAMEALLOC_BUFFER buf_index)
{
    int32_t aligned_size;

    aligned_size = (alloc_size + 3) & ~3;

    if (aligned_size > m_GameAllocMemFree) {
        S_ExitSystemFmt(
            "Memory_Alloc(): OUT OF MEMORY %s %d", m_BufferNames[buf_index],
            aligned_size);
    }

    void *result = m_GameAllocMemPointer;
    m_GameAllocMemFree -= aligned_size;
    m_GameAllocMemPointer += aligned_size;
    return result;
}

void Memory_Free(int32_t free_size, int32_t type)
{
    m_GameAllocMemPointer -= free_size;
    m_GameAllocMemFree += free_size;
    m_GameAllocMemPointer -= free_size;
}
