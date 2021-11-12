#ifndef T1M_GAME_MEMORY_H
#define T1M_GAME_MEMORY_H

#include <stdint.h>

// Internal game memory manager. It allocates its internal buffer once per
// level launch. All subsequent "allocation" requests operate with pointer
// arithmetic. This makes it fast and convenient to request more memory as we
// go, but it makes freeing memory really inconvenient which is why it is
// intentionally not implemented. To use more dynamic memory management, use
// S_Memory_Alloc / S_Memory_Free.

typedef enum MEMORY_BUFFER {
    MEM_BUF_TEXTURE_PAGES,
    MEM_BUF_OBJECT_TEXTURES,
    MEM_BUF_MESH_POINTERS,
    MEM_BUF_MESHES,
    MEM_BUF_ANIMS,
    MEM_BUF_ANIM_CHANGES,
    MEM_BUF_ANIM_RANGES,
    MEM_BUF_ANIM_COMMANDS,
    MEM_BUF_ANIM_BONES,
    MEM_BUF_ANIM_FRAMES,
    MEM_BUF_ROOM_TEXTURES,
    MEM_BUF_ROOM_INFOS,
    MEM_BUF_ROOM_MESH,
    MEM_BUF_ROOM_DOOR,
    MEM_BUF_ROOM_FLOOR,
    MEM_BUF_ROOM_LIGHTS,
    MEM_BUF_ROOM_STATIC_MESH_INFOS,
    MEM_BUF_FLOOR_DATA,
    MEM_BUF_ITEMS,
    MEM_BUF_CAMERAS,
    MEM_BUF_SOUND_FX,
    MEM_BUF_BOXES,
    MEM_BUF_OVERLAPS,
    MEM_BUF_GROUNDZONE,
    MEM_BUF_FLYZONE,
    MEM_BUF_ANIMATING_TEXTURE_RANGES,
    MEM_BUF_CINEMATIC_FRAMES,
    MEM_BUF_LOADDEMO_BUFFER,
    MEM_BUF_SAVEDEMO_BUFFER,
    MEM_BUF_CINEMATIC_EFFECTS,
    MEM_BUF_MUMMY_HEAD_TURN,
    MEM_BUF_EXTRA_DOOR_STUFF,
    MEM_BUF_EFFECTS,
    MEM_BUF_CREATURE_DATA,
    MEM_BUF_CREATURE_LOT,
    MEM_BUF_SAMPLE_INFOS,
    MEM_BUF_SAMPLES,
    MEM_BUF_SAMPLE_OFFSETS,
    MEM_BUF_ROLLINGBALL_STUFF,
} MEMORY_BUFFER;

void Memory_Init();
void *Memory_Alloc(int32_t alloc_size, MEMORY_BUFFER buffer);
void Memory_Shutdown();

#endif
