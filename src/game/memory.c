#include "game/memory.h"

#include "specific/init.h"
#include "specific/memory.h"

#define MALLOC_SIZE 0x1000000 // 16 MB

static char *m_GameMemoryPointer = NULL;
static char *m_GameAllocMemPointer = NULL;
static uint32_t m_GameAllocMemFree = 0;

static const char *m_BufferNames[] = {
    "Sprite Textures", // MEM_BUF_TEXTURE_PAGES
    "Object Textures", // MEM_BUF_OBJECT_TEXTURES
    "Mesh Pointers", // MEM_BUF_MESH_POINTERS
    "Meshes", // MEM_BUF_MESHES
    "Anims", // MEM_BUF_ANIMS
    "Structs", // MEM_BUF_ANIM_CHANGES
    "Ranges", // MEM_BUF_ANIM_RANGES
    "Commands", // MEM_BUF_ANIM_COMMANDS
    "Bones", // MEM_BUF_ANIM_BONES
    "Frames", // MEM_BUF_ANIM_FRAMES
    "Room Textures", // MEM_BUF_ROOM_TEXTURES
    "Room Infos", // MEM_BUF_ROOM_INFOS
    "Room Mesh", // MEM_BUF_ROOM_MESH
    "Room Door", // MEM_BUF_ROOM_DOOR
    "Room Floor", // MEM_BUF_ROOM_FLOOR
    "Room Lights", // MEM_BUF_ROOM_LIGHTS
    "Room Static Mesh Infos", // MEM_BUF_ROOM_STATIC_MESH_INFOS
    "Floor Data", // MEM_BUF_FLOOR_DATA
    "ITEMS!!", // MEM_BUF_ITEMS
    "Cameras", // MEM_BUF_CAMERAS
    "Sound FX", // MEM_BUF_SOUND_FX
    "Boxes", // MEM_BUF_BOXES
    "Overlaps", // MEM_BUF_OVERLAPS
    "GroundZone", // MEM_BUF_GROUNDZONE
    "FlyZone", // MEM_BUF_FLYZONE
    "Animating Texture Ranges", // MEM_BUF_ANIMATING_TEXTURE_RANGES
    "Cinematic Frames", // MEM_BUF_CINEMATIC_FRAMES
    "LoadDemo Buffer", // MEM_BUF_LOADDEMO_BUFFER
    "SaveDemo Buffer", // MEM_BUF_SAVEDEMO_BUFFER
    "Cinematic Effects", // MEM_BUF_CINEMATIC_EFFECTS
    "Mummy Head Turn", // MEM_BUF_MUMMY_HEAD_TURN
    "Extra Door stuff", // MEM_BUF_EXTRA_DOOR_STUFF
    "Effects_Array", // MEM_BUF_EFFECTS
    "Creature Data", // MEM_BUF_CREATURE_DATA
    "Creature LOT", // MEM_BUF_CREATURE_LOT
    "Sample Infos", // MEM_BUF_SAMPLE_INFOS
    "Samples", // MEM_BUF_SAMPLES
    "Sample Offsets", // MEM_BUF_SAMPLE_OFFSETS
    "Rolling Ball Stuff", // MEM_BUF_ROLLINGBALL_STUFF
};

void Memory_Init()
{
    m_GameMemoryPointer = S_Memory_Alloc(MALLOC_SIZE);
    m_GameAllocMemPointer = m_GameMemoryPointer;
    m_GameAllocMemFree = MALLOC_SIZE;
}

void Memory_Shutdown()
{
    S_Memory_Free(m_GameMemoryPointer);
    m_GameMemoryPointer = NULL;
    m_GameAllocMemPointer = NULL;
    m_GameAllocMemFree = 0;
}

void *Memory_Alloc(int32_t alloc_size, MEMORY_BUFFER buffer)
{
    int32_t aligned_size;

    aligned_size = (alloc_size + 3) & ~3;

    if (aligned_size > m_GameAllocMemFree) {
        S_ExitSystemFmt(
            "Memory_Alloc(): OUT OF MEMORY %s %d", m_BufferNames[buffer],
            aligned_size);
    }

    void *result = m_GameAllocMemPointer;
    m_GameAllocMemFree -= aligned_size;
    m_GameAllocMemPointer += aligned_size;
    return result;
}
