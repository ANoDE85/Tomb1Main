#include "game/savegame.h"

#include "filesystem.h"
#include "game/ai/pod.h"
#include "game/control.h"
#include "game/gamebuf.h"
#include "game/gameflow.h"
#include "game/inv.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/lot.h"
#include "game/objects/pickup.h"
#include "game/objects/puzzle_hole.h"
#include "game/shell.h"
#include "game/traps/movable_block.h"
#include "game/traps/rolling_block.h"
#include "global/const.h"
#include "global/vars.h"
#include "log.h"

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stddef.h>

// Loading a saved game is divided into two phases. First, the game reads the
// savegame file contents to look for the level number. The rest of the save
// data is stored in a special buffer in the g_GameInfo. Then the engine
// continues to execute the normal game flow and loads the specified level.
// Second phase occurs after everything finishes loading, e.g. items,
// creatures, triggers etc., and is what actually sets Lara's health, creatures
// status, triggers, inventory etc.

#define SAVEGAME_TITLE_SIZE 75
#define SAVE_CREATURE (1 << 7)

typedef struct SAVEGAME_ITEM_STATS {
    uint8_t num_pickup1;
    uint8_t num_pickup2;
    uint8_t num_puzzle1;
    uint8_t num_puzzle2;
    uint8_t num_puzzle3;
    uint8_t num_puzzle4;
    uint8_t num_key1;
    uint8_t num_key2;
    uint8_t num_key3;
    uint8_t num_key4;
    uint8_t num_leadbar;
    uint8_t dummy;
} SAVEGAME_ITEM_STATS;

static int m_SGBufPos = 0;
static char *m_SGBufPtr = NULL;

static bool SaveGame_NeedsEvilLaraFix();

static void SaveGame_ResetSG(GAME_INFO *game_info);
static void SaveGame_SkipSG(int size);

static void SaveGame_ReadSG(void *pointer, int size);
static void SaveGame_ReadSGARM(LARA_ARM *arm);
static void SaveGame_ReadSGLara(LARA_INFO *lara);
static void SaveGame_ReadSGLOT(LOT_INFO *lot);

static void SaveGame_WriteSG(void *pointer, int size);
static void SaveGame_WriteSGARM(LARA_ARM *arm);
static void SaveGame_WriteSGLara(LARA_INFO *lara);
static void SaveGame_WriteSGLOT(LOT_INFO *lot);

static void SaveGame_FillSaveBuffer(GAME_INFO *game_info);

static bool SaveGame_NeedsEvilLaraFix(GAME_INFO *game_info)
{
    // Heuristic for issue #261.
    // Tomb1Main enables save_flags for Evil Lara, but OG TombATI does not. As
    // a consequence, Atlantis saves made with OG TombATI (which includes the
    // ones available for download on Stella's website) have different layout
    // than the saves made with Tomb1Main. This was discovered after it was too
    // late to make a backwards incompatible change. At the same time, enabling
    // save_flags for Evil Lara is desirable, as not doing this causes her to
    // freeze when the player reloads a save made in her room. This function is
    // used to determine whether the save about to be loaded includes
    // save_flags for Evil Lara or not. Since savegames only contain very
    // concise information, we must make an educated guess here.

    assert(game_info);

    bool result = false;
    if (g_CurrentLevel != 14) {
        return result;
    }

    SaveGame_ResetSG(game_info);
    SaveGame_SkipSG(SAVEGAME_TITLE_SIZE); // level title
    SaveGame_SkipSG(sizeof(int32_t)); // save counter
    for (int i = 0; i < g_GameFlow.level_count; i++) {
        SaveGame_SkipSG(sizeof(uint16_t)); // pistol ammo
        SaveGame_SkipSG(sizeof(uint16_t)); // magnum ammo
        SaveGame_SkipSG(sizeof(uint16_t)); // uzi ammo
        SaveGame_SkipSG(sizeof(uint16_t)); // shotgun ammo
        SaveGame_SkipSG(sizeof(uint8_t)); // small medis
        SaveGame_SkipSG(sizeof(uint8_t)); // big medis
        SaveGame_SkipSG(sizeof(uint8_t)); // scions
        SaveGame_SkipSG(sizeof(int8_t)); // gun status
        SaveGame_SkipSG(sizeof(int8_t)); // gun type
        SaveGame_SkipSG(sizeof(uint16_t)); // flags
    }
    SaveGame_SkipSG(sizeof(uint32_t)); // timer
    SaveGame_SkipSG(sizeof(uint32_t)); // kills
    SaveGame_SkipSG(sizeof(uint16_t)); // secrets
    SaveGame_SkipSG(sizeof(uint16_t)); // current level
    SaveGame_SkipSG(sizeof(uint8_t)); // pickups
    SaveGame_SkipSG(sizeof(uint8_t)); // bonus_flag
    SaveGame_SkipSG(sizeof(SAVEGAME_ITEM_STATS)); // item stats
    SaveGame_SkipSG(sizeof(int32_t)); // flipmap status
    SaveGame_SkipSG(MAX_FLIP_MAPS * sizeof(int8_t)); // flipmap table
    SaveGame_SkipSG(g_NumberCameras * sizeof(int16_t)); // cameras

    for (int i = 0; i < g_LevelItemCount; i++) {
        ITEM_INFO *item = &g_Items[i];
        OBJECT_INFO *obj = &g_Objects[item->object_number];

        ITEM_INFO tmp_item;

        if (obj->save_position) {
            SaveGame_ReadSG(&tmp_item.pos, sizeof(PHD_3DPOS));
            SaveGame_SkipSG(sizeof(int16_t));
            SaveGame_ReadSG(&tmp_item.speed, sizeof(int16_t));
            SaveGame_ReadSG(&tmp_item.fall_speed, sizeof(int16_t));
        }
        if (obj->save_anim) {
            SaveGame_ReadSG(&tmp_item.current_anim_state, sizeof(int16_t));
            SaveGame_ReadSG(&tmp_item.goal_anim_state, sizeof(int16_t));
            SaveGame_ReadSG(&tmp_item.required_anim_state, sizeof(int16_t));
            SaveGame_ReadSG(&tmp_item.anim_number, sizeof(int16_t));
            SaveGame_ReadSG(&tmp_item.frame_number, sizeof(int16_t));
        }
        if (obj->save_hitpoints) {
            SaveGame_ReadSG(&tmp_item.hit_points, sizeof(int16_t));
        }
        if (obj->save_flags) {
            SaveGame_ReadSG(&tmp_item.flags, sizeof(int16_t));
            SaveGame_ReadSG(&tmp_item.timer, sizeof(int16_t));
            if (tmp_item.flags & SAVE_CREATURE) {
                CREATURE_INFO tmp_creature;
                SaveGame_ReadSG(&tmp_creature.head_rotation, sizeof(int16_t));
                SaveGame_ReadSG(&tmp_creature.neck_rotation, sizeof(int16_t));
                SaveGame_ReadSG(&tmp_creature.maximum_turn, sizeof(int16_t));
                SaveGame_ReadSG(&tmp_creature.flags, sizeof(int16_t));
                SaveGame_ReadSG(&tmp_creature.mood, sizeof(int32_t));
            }
        }

        // check for exceptionally high item positions.
        if ((ABS(tmp_item.pos.x) | ABS(tmp_item.pos.y) | ABS(tmp_item.pos.z))
            & 0xFF000000) {
            result = true;
        }
    }

    return result;
}

void InitialiseStartInfo()
{
    for (int i = 0; i < g_GameFlow.level_count; i++) {
        ModifyStartInfo(i);
        g_GameInfo.start[i].flags.available = 0;
    }
    g_GameInfo.start[g_GameFlow.gym_level_num].flags.available = 1;
    g_GameInfo.start[g_GameFlow.first_level_num].flags.available = 1;
}

void ModifyStartInfo(int32_t level_num)
{
    START_INFO *start = &g_GameInfo.start[level_num];

    start->flags.got_pistols = 1;
    start->gun_type = LGT_PISTOLS;
    start->pistol_ammo = 1000;

    if (level_num == g_GameFlow.gym_level_num) {
        start->flags.available = 1;
        start->flags.costume = 1;
        start->num_medis = 0;
        start->num_big_medis = 0;
        start->num_scions = 0;
        start->pistol_ammo = 0;
        start->shotgun_ammo = 0;
        start->magnum_ammo = 0;
        start->uzi_ammo = 0;
        start->flags.got_pistols = 0;
        start->flags.got_shotgun = 0;
        start->flags.got_magnums = 0;
        start->flags.got_uzis = 0;
        start->gun_type = LGT_UNARMED;
        start->gun_status = LGS_ARMLESS;
    }

    if (level_num == g_GameFlow.first_level_num) {
        start->flags.available = 1;
        start->flags.costume = 0;
        start->num_medis = 0;
        start->num_big_medis = 0;
        start->num_scions = 0;
        start->shotgun_ammo = 0;
        start->magnum_ammo = 0;
        start->uzi_ammo = 0;
        start->flags.got_shotgun = 0;
        start->flags.got_magnums = 0;
        start->flags.got_uzis = 0;
        start->gun_status = LGS_ARMLESS;
    }

    if ((g_GameInfo.bonus_flag & GBF_NGPLUS)
        && level_num != g_GameFlow.gym_level_num) {
        start->flags.got_pistols = 1;
        start->flags.got_shotgun = 1;
        start->flags.got_magnums = 1;
        start->flags.got_uzis = 1;
        start->shotgun_ammo = 1234;
        start->magnum_ammo = 1234;
        start->uzi_ammo = 1234;
        start->gun_type = LGT_UZIS;
    }
}

void CreateStartInfo(int level_num)
{
    START_INFO *start = &g_GameInfo.start[level_num];

    start->flags.available = 1;
    start->flags.costume = 0;

    start->pistol_ammo = 1000;
    if (Inv_RequestItem(O_GUN_ITEM)) {
        start->flags.got_pistols = 1;
    } else {
        start->flags.got_pistols = 0;
    }

    if (Inv_RequestItem(O_MAGNUM_ITEM)) {
        start->magnum_ammo = g_Lara.magnums.ammo;
        start->flags.got_magnums = 1;
    } else {
        start->magnum_ammo = Inv_RequestItem(O_MAG_AMMO_ITEM) * MAGNUM_AMMO_QTY;
        start->flags.got_magnums = 0;
    }

    if (Inv_RequestItem(O_UZI_ITEM)) {
        start->uzi_ammo = g_Lara.uzis.ammo;
        start->flags.got_uzis = 1;
    } else {
        start->uzi_ammo = Inv_RequestItem(O_UZI_AMMO_ITEM) * UZI_AMMO_QTY;
        start->flags.got_uzis = 0;
    }

    if (Inv_RequestItem(O_SHOTGUN_ITEM)) {
        start->shotgun_ammo = g_Lara.shotgun.ammo;
        start->flags.got_shotgun = 1;
    } else {
        start->shotgun_ammo =
            Inv_RequestItem(O_SG_AMMO_ITEM) * SHOTGUN_AMMO_QTY;
        start->flags.got_shotgun = 0;
    }

    start->num_medis = Inv_RequestItem(O_MEDI_ITEM);
    start->num_big_medis = Inv_RequestItem(O_BIGMEDI_ITEM);
    start->num_scions = Inv_RequestItem(O_SCION_ITEM);

    start->gun_type = g_Lara.gun_type;
    if (g_Lara.gun_status == LGS_READY) {
        start->gun_status = LGS_READY;
    } else {
        start->gun_status = LGS_ARMLESS;
    }
}

static void SaveGame_FillSaveBuffer(GAME_INFO *game_info)
{
    // Write current game information into the save buffer.

    assert(game_info);

    CreateStartInfo(g_CurrentLevel);

    SaveGame_ResetSG(game_info);
    memset(m_SGBufPtr, 0, MAX_SAVEGAME_BUFFER);

    char title[SAVEGAME_TITLE_SIZE];
    snprintf(
        title, SAVEGAME_TITLE_SIZE, "%s",
        g_GameFlow.levels[g_CurrentLevel].level_title);
    SaveGame_WriteSG(title, SAVEGAME_TITLE_SIZE);
    SaveGame_WriteSG(&g_SaveCounter, sizeof(int32_t));

    for (int i = 0; i < g_GameFlow.level_count; i++) {
        if (g_GameFlow.levels[i].level_type == GFL_CURRENT) {
            game_info->start[i] = game_info->start[g_CurrentLevel];
        }
    }

    assert(game_info->start);
    for (int i = 0; i < g_GameFlow.level_count; i++) {
        START_INFO *start = &game_info->start[i];
        SaveGame_WriteSG(&start->pistol_ammo, sizeof(uint16_t));
        SaveGame_WriteSG(&start->magnum_ammo, sizeof(uint16_t));
        SaveGame_WriteSG(&start->uzi_ammo, sizeof(uint16_t));
        SaveGame_WriteSG(&start->shotgun_ammo, sizeof(uint16_t));
        SaveGame_WriteSG(&start->num_medis, sizeof(uint8_t));
        SaveGame_WriteSG(&start->num_big_medis, sizeof(uint8_t));
        SaveGame_WriteSG(&start->num_scions, sizeof(uint8_t));
        SaveGame_WriteSG(&start->gun_status, sizeof(int8_t));
        SaveGame_WriteSG(&start->gun_type, sizeof(int8_t));
        SaveGame_WriteSG(&start->flags, sizeof(uint16_t));
    }

    SaveGame_WriteSG(&game_info->timer, sizeof(uint32_t));
    SaveGame_WriteSG(&game_info->kills, sizeof(uint32_t));
    SaveGame_WriteSG(&game_info->secrets, sizeof(uint16_t));
    SaveGame_WriteSG(&g_CurrentLevel, sizeof(uint16_t));
    SaveGame_WriteSG(&game_info->pickups, sizeof(uint8_t));
    SaveGame_WriteSG(&game_info->bonus_flag, sizeof(uint8_t));

    SAVEGAME_ITEM_STATS item_stats = {
        .num_pickup1 = Inv_RequestItem(O_PICKUP_ITEM1),
        .num_pickup2 = Inv_RequestItem(O_PICKUP_ITEM2),
        .num_puzzle1 = Inv_RequestItem(O_PUZZLE_ITEM1),
        .num_puzzle2 = Inv_RequestItem(O_PUZZLE_ITEM2),
        .num_puzzle3 = Inv_RequestItem(O_PUZZLE_ITEM3),
        .num_puzzle4 = Inv_RequestItem(O_PUZZLE_ITEM4),
        .num_key1 = Inv_RequestItem(O_KEY_ITEM1),
        .num_key2 = Inv_RequestItem(O_KEY_ITEM2),
        .num_key3 = Inv_RequestItem(O_KEY_ITEM3),
        .num_key4 = Inv_RequestItem(O_KEY_ITEM4),
        .num_leadbar = Inv_RequestItem(O_LEADBAR_ITEM),
        0
    };

    SaveGame_WriteSG(&item_stats, sizeof(item_stats));

    SaveGame_WriteSG(&g_FlipStatus, sizeof(int32_t));
    for (int i = 0; i < MAX_FLIP_MAPS; i++) {
        int8_t flag = g_FlipMapTable[i] >> 8;
        SaveGame_WriteSG(&flag, sizeof(int8_t));
    }

    for (int i = 0; i < g_NumberCameras; i++) {
        SaveGame_WriteSG(&g_Camera.fixed[i].flags, sizeof(int16_t));
    }

    for (int i = 0; i < g_LevelItemCount; i++) {
        ITEM_INFO *item = &g_Items[i];
        OBJECT_INFO *obj = &g_Objects[item->object_number];

        if (obj->save_position) {
            SaveGame_WriteSG(&item->pos, sizeof(PHD_3DPOS));
            SaveGame_WriteSG(&item->room_number, sizeof(int16_t));
            SaveGame_WriteSG(&item->speed, sizeof(int16_t));
            SaveGame_WriteSG(&item->fall_speed, sizeof(int16_t));
        }

        if (obj->save_anim) {
            SaveGame_WriteSG(&item->current_anim_state, sizeof(int16_t));
            SaveGame_WriteSG(&item->goal_anim_state, sizeof(int16_t));
            SaveGame_WriteSG(&item->required_anim_state, sizeof(int16_t));
            SaveGame_WriteSG(&item->anim_number, sizeof(int16_t));
            SaveGame_WriteSG(&item->frame_number, sizeof(int16_t));
        }

        if (obj->save_hitpoints) {
            SaveGame_WriteSG(&item->hit_points, sizeof(int16_t));
        }

        if (obj->save_flags) {
            uint16_t flags = item->flags + item->active + (item->status << 1)
                + (item->gravity_status << 3) + (item->collidable << 4);
            if (obj->intelligent && item->data) {
                flags |= SAVE_CREATURE;
            }
            SaveGame_WriteSG(&flags, sizeof(uint16_t));
            SaveGame_WriteSG(&item->timer, sizeof(int16_t));
            if (flags & SAVE_CREATURE) {
                CREATURE_INFO *creature = item->data;
                SaveGame_WriteSG(&creature->head_rotation, sizeof(int16_t));
                SaveGame_WriteSG(&creature->neck_rotation, sizeof(int16_t));
                SaveGame_WriteSG(&creature->maximum_turn, sizeof(int16_t));
                SaveGame_WriteSG(&creature->flags, sizeof(int16_t));
                SaveGame_WriteSG(&creature->mood, sizeof(int32_t));
            }
        }
    }

    SaveGame_WriteSGLara(&g_Lara);

    SaveGame_WriteSG(&g_FlipEffect, sizeof(int32_t));
    SaveGame_WriteSG(&g_FlipTimer, sizeof(int32_t));
}

void SaveGame_ApplySaveBuffer(GAME_INFO *game_info)
{
    // Write current game information into the save buffer.

    assert(game_info);

    int8_t tmp8;
    int16_t tmp16;
    int32_t tmp32;

    bool skip_reading_evil_lara = SaveGame_NeedsEvilLaraFix(game_info);
    if (skip_reading_evil_lara) {
        LOG_INFO("Enabling Evil Lara savegame fix");
    }

    SaveGame_ResetSG(game_info);
    SaveGame_SkipSG(SAVEGAME_TITLE_SIZE); // level title
    SaveGame_SkipSG(sizeof(int32_t)); // save counter

    assert(game_info->start);
    for (int i = 0; i < g_GameFlow.level_count; i++) {
        START_INFO *start = &game_info->start[i];
        SaveGame_ReadSG(&start->pistol_ammo, sizeof(uint16_t));
        SaveGame_ReadSG(&start->magnum_ammo, sizeof(uint16_t));
        SaveGame_ReadSG(&start->uzi_ammo, sizeof(uint16_t));
        SaveGame_ReadSG(&start->shotgun_ammo, sizeof(uint16_t));
        SaveGame_ReadSG(&start->num_medis, sizeof(uint8_t));
        SaveGame_ReadSG(&start->num_big_medis, sizeof(uint8_t));
        SaveGame_ReadSG(&start->num_scions, sizeof(uint8_t));
        SaveGame_ReadSG(&start->gun_status, sizeof(int8_t));
        SaveGame_ReadSG(&start->gun_type, sizeof(int8_t));
        SaveGame_ReadSG(&start->flags, sizeof(uint16_t));
    }

    SaveGame_ReadSG(&game_info->timer, sizeof(uint32_t));
    SaveGame_ReadSG(&game_info->kills, sizeof(uint32_t));
    SaveGame_ReadSG(&game_info->secrets, sizeof(uint16_t));
    SaveGame_ReadSG(&g_CurrentLevel, sizeof(uint16_t));
    SaveGame_ReadSG(&game_info->pickups, sizeof(uint8_t));
    SaveGame_ReadSG(&game_info->bonus_flag, sizeof(uint8_t));

    for (int i = 0; i < g_GameFlow.level_count; i++) {
        if (g_GameFlow.levels[i].level_type == GFL_CURRENT) {
            game_info->start[g_CurrentLevel] = game_info->start[i];
        }
    }

    InitialiseLaraInventory(g_CurrentLevel);
    SAVEGAME_ITEM_STATS item_stats = { 0 };
    SaveGame_ReadSG(&item_stats, sizeof(item_stats));
    Inv_AddItemNTimes(O_PICKUP_ITEM1, item_stats.num_pickup1);
    Inv_AddItemNTimes(O_PICKUP_ITEM2, item_stats.num_pickup2);
    Inv_AddItemNTimes(O_PUZZLE_ITEM1, item_stats.num_puzzle1);
    Inv_AddItemNTimes(O_PUZZLE_ITEM2, item_stats.num_puzzle2);
    Inv_AddItemNTimes(O_PUZZLE_ITEM3, item_stats.num_puzzle3);
    Inv_AddItemNTimes(O_PUZZLE_ITEM4, item_stats.num_puzzle4);
    Inv_AddItemNTimes(O_KEY_ITEM1, item_stats.num_key1);
    Inv_AddItemNTimes(O_KEY_ITEM2, item_stats.num_key2);
    Inv_AddItemNTimes(O_KEY_ITEM3, item_stats.num_key3);
    Inv_AddItemNTimes(O_KEY_ITEM4, item_stats.num_key4);
    Inv_AddItemNTimes(O_LEADBAR_ITEM, item_stats.num_leadbar);

    SaveGame_ReadSG(&tmp32, sizeof(int32_t));
    if (tmp32) {
        FlipMap();
    }

    for (int i = 0; i < MAX_FLIP_MAPS; i++) {
        SaveGame_ReadSG(&tmp8, sizeof(int8_t));
        g_FlipMapTable[i] = tmp8 << 8;
    }

    for (int i = 0; i < g_NumberCameras; i++) {
        SaveGame_ReadSG(&g_Camera.fixed[i].flags, sizeof(int16_t));
    }

    for (int i = 0; i < g_LevelItemCount; i++) {
        ITEM_INFO *item = &g_Items[i];
        OBJECT_INFO *obj = &g_Objects[item->object_number];

        if (obj->control == MovableBlockControl) {
            AlterFloorHeight(item, WALL_L);
        }
        if (obj->control == RollingBlockControl) {
            AlterFloorHeight(item, WALL_L * 2);
        }

        if (obj->save_position) {
            SaveGame_ReadSG(&item->pos, sizeof(PHD_3DPOS));
            SaveGame_ReadSG(&tmp16, sizeof(int16_t));
            SaveGame_ReadSG(&item->speed, sizeof(int16_t));
            SaveGame_ReadSG(&item->fall_speed, sizeof(int16_t));

            if (item->room_number != tmp16) {
                ItemNewRoom(i, tmp16);
            }

            if (obj->shadow_size) {
                FLOOR_INFO *floor =
                    GetFloor(item->pos.x, item->pos.y, item->pos.z, &tmp16);
                item->floor =
                    GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);
            }
        }

        if (obj->save_anim) {
            SaveGame_ReadSG(&item->current_anim_state, sizeof(int16_t));
            SaveGame_ReadSG(&item->goal_anim_state, sizeof(int16_t));
            SaveGame_ReadSG(&item->required_anim_state, sizeof(int16_t));
            SaveGame_ReadSG(&item->anim_number, sizeof(int16_t));
            SaveGame_ReadSG(&item->frame_number, sizeof(int16_t));
        }

        if (obj->save_hitpoints) {
            SaveGame_ReadSG(&item->hit_points, sizeof(int16_t));
        }

        if (obj->save_flags
            && (item->object_number != O_EVIL_LARA
                || !skip_reading_evil_lara)) {
            SaveGame_ReadSG(&item->flags, sizeof(int16_t));
            SaveGame_ReadSG(&item->timer, sizeof(int16_t));

            if (item->flags & IF_KILLED_ITEM) {
                KillItem(i);
                item->status = IS_DEACTIVATED;
            } else {
                if ((item->flags & 1) && !item->active) {
                    AddActiveItem(i);
                }
                item->status = (item->flags & 6) >> 1;
                if (item->flags & 8) {
                    item->gravity_status = 1;
                }
                if (!(item->flags & 16)) {
                    item->collidable = 0;
                }
            }

            if (item->flags & SAVE_CREATURE) {
                EnableBaddieAI(i, 1);
                CREATURE_INFO *creature = item->data;
                if (creature) {
                    SaveGame_ReadSG(&creature->head_rotation, sizeof(int16_t));
                    SaveGame_ReadSG(&creature->neck_rotation, sizeof(int16_t));
                    SaveGame_ReadSG(&creature->maximum_turn, sizeof(int16_t));
                    SaveGame_ReadSG(&creature->flags, sizeof(int16_t));
                    SaveGame_ReadSG(&creature->mood, sizeof(int32_t));
                } else {
                    SaveGame_SkipSG(4 * 2 + 4);
                }
            } else if (obj->intelligent) {
                item->data = NULL;
            }

            item->flags &= 0xFF00;

            if (obj->collision == PuzzleHoleCollision
                && (item->status == IS_DEACTIVATED
                    || item->status == IS_ACTIVE)) {
                item->object_number += O_PUZZLE_DONE1 - O_PUZZLE_HOLE1;
            }

            if (obj->control == PodControl && item->status == IS_DEACTIVATED) {
                item->mesh_bits = 0x1FF;
                item->collidable = 0;
            }

            if (obj->collision == PickUpCollision
                && item->status == IS_DEACTIVATED) {
                RemoveDrawnItem(i);
            }
        }

        if (obj->control == MovableBlockControl
            && item->status == IS_NOT_ACTIVE) {
            AlterFloorHeight(item, -WALL_L);
        }

        if (obj->control == RollingBlockControl
            && item->current_anim_state != RBS_MOVING) {
            AlterFloorHeight(item, -WALL_L * 2);
        }

        if (item->object_number == O_PIERRE && item->hit_points <= 0
            && (item->flags & IF_ONESHOT)) {
            if (Inv_RequestItem(O_SCION_ITEM) == 1) {
                SpawnItem(item, O_MAGNUM_ITEM);
                SpawnItem(item, O_SCION_ITEM2);
                SpawnItem(item, O_KEY_ITEM1);
            }
            g_MusicTrackFlags[MX_PIERRE_SPEECH] |= IF_ONESHOT;
        }

        if (item->object_number == O_MERCENARY1 && item->hit_points <= 0) {
            if (!Inv_RequestItem(O_UZI_ITEM)) {
                SpawnItem(item, O_UZI_ITEM);
            }
        }

        if (item->object_number == O_MERCENARY2 && item->hit_points <= 0) {
            if (!Inv_RequestItem(O_MAGNUM_ITEM)) {
                SpawnItem(item, O_MAGNUM_ITEM);
            }
            g_MusicTrackFlags[MX_COWBOY_SPEECH] |= IF_ONESHOT;
        }

        if (item->object_number == O_MERCENARY3 && item->hit_points <= 0) {
            if (!Inv_RequestItem(O_SHOTGUN_ITEM)) {
                SpawnItem(item, O_SHOTGUN_ITEM);
            }
            g_MusicTrackFlags[MX_BALDY_SPEECH] |= IF_ONESHOT;
        }

        if (item->object_number == O_LARSON && item->hit_points <= 0) {
            g_MusicTrackFlags[MX_BALDY_SPEECH] |= IF_ONESHOT;
        }
    }

    BOX_NODE *node = g_Lara.LOT.node;
    SaveGame_ReadSGLara(&g_Lara);
    g_Lara.LOT.node = node;
    g_Lara.LOT.target_box = NO_BOX;

    SaveGame_ReadSG(&g_FlipEffect, sizeof(int32_t));
    SaveGame_ReadSG(&g_FlipTimer, sizeof(int32_t));
}

static void SaveGame_ResetSG(GAME_INFO *game_info)
{
    assert(game_info);
    m_SGBufPos = 0;
    m_SGBufPtr = game_info->savegame_buffer;
}

static void SaveGame_SkipSG(int size)
{
    m_SGBufPtr += size;
    m_SGBufPos += size; // missing from OG
}

static void SaveGame_WriteSG(void *pointer, int size)
{
    m_SGBufPos += size;
    if (m_SGBufPos >= MAX_SAVEGAME_BUFFER) {
        Shell_ExitSystem("FATAL: Savegame is too big to fit in buffer");
    }

    char *data = (char *)pointer;
    for (int i = 0; i < size; i++) {
        *m_SGBufPtr++ = *data++;
    }
}

static void SaveGame_WriteSGLara(LARA_INFO *lara)
{
    int32_t tmp32 = 0;

    SaveGame_WriteSG(&lara->item_number, sizeof(int16_t));
    SaveGame_WriteSG(&lara->gun_status, sizeof(int16_t));
    SaveGame_WriteSG(&lara->gun_type, sizeof(int16_t));
    SaveGame_WriteSG(&lara->request_gun_type, sizeof(int16_t));
    SaveGame_WriteSG(&lara->calc_fall_speed, sizeof(int16_t));
    SaveGame_WriteSG(&lara->water_status, sizeof(int16_t));
    SaveGame_WriteSG(&lara->pose_count, sizeof(int16_t));
    SaveGame_WriteSG(&lara->hit_frame, sizeof(int16_t));
    SaveGame_WriteSG(&lara->hit_direction, sizeof(int16_t));
    SaveGame_WriteSG(&lara->air, sizeof(int16_t));
    SaveGame_WriteSG(&lara->dive_count, sizeof(int16_t));
    SaveGame_WriteSG(&lara->death_count, sizeof(int16_t));
    SaveGame_WriteSG(&lara->current_active, sizeof(int16_t));
    SaveGame_WriteSG(&lara->spaz_effect_count, sizeof(int16_t));

    // OG just writes the pointer address (!)
    if (lara->spaz_effect) {
        tmp32 = (size_t)lara->spaz_effect - (size_t)g_Effects;
    }
    SaveGame_WriteSG(&tmp32, sizeof(int32_t));

    SaveGame_WriteSG(&lara->mesh_effects, sizeof(int32_t));

    for (int i = 0; i < LM_NUMBER_OF; i++) {
        tmp32 = (size_t)lara->mesh_ptrs[i] - (size_t)g_MeshBase;
        SaveGame_WriteSG(&tmp32, sizeof(int32_t));
    }

    // OG just writes the pointer address (!) assuming it's a non-existing mesh
    // 16 (!!) which happens to be g_Lara's current target. Just write NULL.
    tmp32 = 0;
    SaveGame_WriteSG(&tmp32, sizeof(int32_t));

    SaveGame_WriteSG(&lara->target_angles[0], sizeof(PHD_ANGLE));
    SaveGame_WriteSG(&lara->target_angles[1], sizeof(PHD_ANGLE));
    SaveGame_WriteSG(&lara->turn_rate, sizeof(int16_t));
    SaveGame_WriteSG(&lara->move_angle, sizeof(int16_t));
    SaveGame_WriteSG(&lara->head_y_rot, sizeof(int16_t));
    SaveGame_WriteSG(&lara->head_x_rot, sizeof(int16_t));
    SaveGame_WriteSG(&lara->head_z_rot, sizeof(int16_t));
    SaveGame_WriteSG(&lara->torso_y_rot, sizeof(int16_t));
    SaveGame_WriteSG(&lara->torso_x_rot, sizeof(int16_t));
    SaveGame_WriteSG(&lara->torso_z_rot, sizeof(int16_t));

    SaveGame_WriteSGARM(&lara->left_arm);
    SaveGame_WriteSGARM(&lara->right_arm);
    SaveGame_WriteSG(&lara->pistols, sizeof(AMMO_INFO));
    SaveGame_WriteSG(&lara->magnums, sizeof(AMMO_INFO));
    SaveGame_WriteSG(&lara->uzis, sizeof(AMMO_INFO));
    SaveGame_WriteSG(&lara->shotgun, sizeof(AMMO_INFO));
    SaveGame_WriteSGLOT(&lara->LOT);
}

static void SaveGame_WriteSGARM(LARA_ARM *arm)
{
    int32_t frame_base = (size_t)arm->frame_base - (size_t)g_AnimFrames;
    SaveGame_WriteSG(&frame_base, sizeof(int32_t));
    SaveGame_WriteSG(&arm->frame_number, sizeof(int16_t));
    SaveGame_WriteSG(&arm->lock, sizeof(int16_t));
    SaveGame_WriteSG(&arm->y_rot, sizeof(PHD_ANGLE));
    SaveGame_WriteSG(&arm->x_rot, sizeof(PHD_ANGLE));
    SaveGame_WriteSG(&arm->z_rot, sizeof(PHD_ANGLE));
    SaveGame_WriteSG(&arm->flash_gun, sizeof(int16_t));
}

static void SaveGame_WriteSGLOT(LOT_INFO *lot)
{
    // it casually saves a pointer again!
    SaveGame_WriteSG(&lot->node, sizeof(int32_t));

    SaveGame_WriteSG(&lot->head, sizeof(int16_t));
    SaveGame_WriteSG(&lot->tail, sizeof(int16_t));
    SaveGame_WriteSG(&lot->search_number, sizeof(uint16_t));
    SaveGame_WriteSG(&lot->block_mask, sizeof(uint16_t));
    SaveGame_WriteSG(&lot->step, sizeof(int16_t));
    SaveGame_WriteSG(&lot->drop, sizeof(int16_t));
    SaveGame_WriteSG(&lot->fly, sizeof(int16_t));
    SaveGame_WriteSG(&lot->zone_count, sizeof(int16_t));
    SaveGame_WriteSG(&lot->target_box, sizeof(int16_t));
    SaveGame_WriteSG(&lot->required_box, sizeof(int16_t));
    SaveGame_WriteSG(&lot->target, sizeof(PHD_VECTOR));
}

static void SaveGame_ReadSG(void *pointer, int size)
{
    m_SGBufPos += size;
    char *data = (char *)pointer;
    for (int i = 0; i < size; i++)
        *data++ = *m_SGBufPtr++;
}

static void SaveGame_ReadSGLara(LARA_INFO *lara)
{
    int32_t tmp32 = 0;

    SaveGame_ReadSG(&lara->item_number, sizeof(int16_t));
    SaveGame_ReadSG(&lara->gun_status, sizeof(int16_t));
    SaveGame_ReadSG(&lara->gun_type, sizeof(int16_t));
    SaveGame_ReadSG(&lara->request_gun_type, sizeof(int16_t));
    SaveGame_ReadSG(&lara->calc_fall_speed, sizeof(int16_t));
    SaveGame_ReadSG(&lara->water_status, sizeof(int16_t));
    SaveGame_ReadSG(&lara->pose_count, sizeof(int16_t));
    SaveGame_ReadSG(&lara->hit_frame, sizeof(int16_t));
    SaveGame_ReadSG(&lara->hit_direction, sizeof(int16_t));
    SaveGame_ReadSG(&lara->air, sizeof(int16_t));
    SaveGame_ReadSG(&lara->dive_count, sizeof(int16_t));
    SaveGame_ReadSG(&lara->death_count, sizeof(int16_t));
    SaveGame_ReadSG(&lara->current_active, sizeof(int16_t));
    SaveGame_ReadSG(&lara->spaz_effect_count, sizeof(int16_t));

    lara->spaz_effect = NULL;
    SaveGame_SkipSG(sizeof(FX_INFO *));

    SaveGame_ReadSG(&lara->mesh_effects, sizeof(int32_t));
    for (int i = 0; i < LM_NUMBER_OF; i++) {
        SaveGame_ReadSG(&tmp32, sizeof(int32_t));
        lara->mesh_ptrs[i] = (int16_t *)((size_t)g_MeshBase + (size_t)tmp32);
    }

    lara->target = NULL;
    SaveGame_SkipSG(sizeof(ITEM_INFO *));

    SaveGame_ReadSG(&lara->target_angles[0], sizeof(PHD_ANGLE));
    SaveGame_ReadSG(&lara->target_angles[1], sizeof(PHD_ANGLE));
    SaveGame_ReadSG(&lara->turn_rate, sizeof(int16_t));
    SaveGame_ReadSG(&lara->move_angle, sizeof(int16_t));
    SaveGame_ReadSG(&lara->head_y_rot, sizeof(int16_t));
    SaveGame_ReadSG(&lara->head_x_rot, sizeof(int16_t));
    SaveGame_ReadSG(&lara->head_z_rot, sizeof(int16_t));
    SaveGame_ReadSG(&lara->torso_y_rot, sizeof(int16_t));
    SaveGame_ReadSG(&lara->torso_x_rot, sizeof(int16_t));
    SaveGame_ReadSG(&lara->torso_z_rot, sizeof(int16_t));

    SaveGame_ReadSGARM(&lara->left_arm);
    SaveGame_ReadSGARM(&lara->right_arm);
    SaveGame_ReadSG(&lara->pistols, sizeof(AMMO_INFO));
    SaveGame_ReadSG(&lara->magnums, sizeof(AMMO_INFO));
    SaveGame_ReadSG(&lara->uzis, sizeof(AMMO_INFO));
    SaveGame_ReadSG(&lara->shotgun, sizeof(AMMO_INFO));
    SaveGame_ReadSGLOT(&lara->LOT);
}

static void SaveGame_ReadSGARM(LARA_ARM *arm)
{
    int32_t frame_base;
    SaveGame_ReadSG(&frame_base, sizeof(int32_t));
    arm->frame_base = (int16_t *)((size_t)g_AnimFrames + (size_t)frame_base);

    SaveGame_ReadSG(&arm->frame_number, sizeof(int16_t));
    SaveGame_ReadSG(&arm->lock, sizeof(int16_t));
    SaveGame_ReadSG(&arm->y_rot, sizeof(PHD_ANGLE));
    SaveGame_ReadSG(&arm->x_rot, sizeof(PHD_ANGLE));
    SaveGame_ReadSG(&arm->z_rot, sizeof(PHD_ANGLE));
    SaveGame_ReadSG(&arm->flash_gun, sizeof(int16_t));
}

static void SaveGame_ReadSGLOT(LOT_INFO *lot)
{
    lot->node = NULL;
    SaveGame_SkipSG(sizeof(BOX_NODE *));

    SaveGame_ReadSG(&lot->head, sizeof(int16_t));
    SaveGame_ReadSG(&lot->tail, sizeof(int16_t));
    SaveGame_ReadSG(&lot->search_number, sizeof(uint16_t));
    SaveGame_ReadSG(&lot->block_mask, sizeof(uint16_t));
    SaveGame_ReadSG(&lot->step, sizeof(int16_t));
    SaveGame_ReadSG(&lot->drop, sizeof(int16_t));
    SaveGame_ReadSG(&lot->fly, sizeof(int16_t));
    SaveGame_ReadSG(&lot->zone_count, sizeof(int16_t));
    SaveGame_ReadSG(&lot->target_box, sizeof(int16_t));
    SaveGame_ReadSG(&lot->required_box, sizeof(int16_t));
    SaveGame_ReadSG(&lot->target, sizeof(PHD_VECTOR));
}

int16_t SaveGame_LoadSaveBufferFromFile(GAME_INFO *game_info, int32_t slot)
{
    assert(game_info);

    char filename[80];
    sprintf(filename, g_GameFlow.save_game_fmt, slot);
    LOG_DEBUG("%s", filename);

    MYFILE *fp = File_Open(filename, FILE_OPEN_READ);
    if (!fp) {
        return -1;
    }
    File_Read(
        &game_info->savegame_buffer[0], sizeof(char), MAX_SAVEGAME_BUFFER, fp);
    File_Close(fp);

    SaveGame_ResetSG(game_info);
    SaveGame_SkipSG(SAVEGAME_TITLE_SIZE); // level title
    SaveGame_SkipSG(sizeof(int32_t)); // save counter
    for (int i = 0; i < g_GameFlow.level_count; i++) {
        SaveGame_SkipSG(sizeof(uint16_t)); // pistol ammo
        SaveGame_SkipSG(sizeof(uint16_t)); // magnum ammo
        SaveGame_SkipSG(sizeof(uint16_t)); // uzi ammo
        SaveGame_SkipSG(sizeof(uint16_t)); // shotgun ammo
        SaveGame_SkipSG(sizeof(uint8_t)); // small medis
        SaveGame_SkipSG(sizeof(uint8_t)); // big medis
        SaveGame_SkipSG(sizeof(uint8_t)); // scions
        SaveGame_SkipSG(sizeof(int8_t)); // gun status
        SaveGame_SkipSG(sizeof(int8_t)); // gun type
        SaveGame_SkipSG(sizeof(uint16_t)); // flags
    }
    SaveGame_SkipSG(sizeof(uint32_t)); // timer
    SaveGame_SkipSG(sizeof(uint32_t)); // kills
    SaveGame_SkipSG(sizeof(uint16_t)); // secrets

    uint16_t level_num;
    SaveGame_ReadSG(&level_num, sizeof(int16_t));

    return level_num;
}

bool SaveGame_SaveToFile(GAME_INFO *game_info, int32_t slot)
{
    assert(game_info);
    SaveGame_FillSaveBuffer(game_info);

    char filename[80];
    sprintf(filename, g_GameFlow.save_game_fmt, slot);
    LOG_DEBUG("%s", filename);

    MYFILE *fp = File_Open(filename, FILE_OPEN_WRITE);
    if (!fp) {
        return false;
    }
    File_Write(&game_info->savegame_buffer[0], sizeof(char), m_SGBufPos, fp);
    File_Close(fp);

    REQUEST_INFO *req = &g_LoadSaveGameRequester;
    req->item_flags[slot] &= ~RIF_BLOCKED;
    sprintf(
        &req->item_texts[req->item_text_len * slot], "%s %d",
        g_GameFlow.levels[g_CurrentLevel].level_title, g_SaveCounter);
    g_SavedGamesCount++;
    g_SaveCounter++;
    return true;
}

void SaveGame_ScanSavedGames()
{
    REQUEST_INFO *req = &g_LoadSaveGameRequester;

    req->items = 0;
    g_SaveCounter = 0;
    g_SavedGamesCount = 0;
    for (int i = 0; i < MAX_SAVE_SLOTS; i++) {
        char filename[80];
        sprintf(filename, g_GameFlow.save_game_fmt, i);

        MYFILE *fp = File_Open(filename, FILE_OPEN_READ);
        if (fp) {
            File_Read(filename, sizeof(char), SAVEGAME_TITLE_SIZE, fp);
            int32_t counter;
            File_Read(&counter, sizeof(int32_t), 1, fp);
            File_Close(fp);

            req->item_flags[req->items] &= ~RIF_BLOCKED;

            sprintf(
                &req->item_texts[req->items * req->item_text_len], "%s %d",
                filename, counter);

            if (counter > g_SaveCounter) {
                g_SaveCounter = counter;
                req->requested = i;
            }

            g_SavedGamesCount++;
        } else {
            req->item_flags[req->items] |= RIF_BLOCKED;

            sprintf(
                &req->item_texts[req->items * req->item_text_len],
                g_GameFlow.strings[GS_MISC_EMPTY_SLOT_FMT], i + 1);
        }

        req->items++;
    }

    if (req->requested >= req->vis_lines) {
        req->line_offset = req->requested - req->vis_lines + 1;
    } else if (req->requested < req->line_offset) {
        req->line_offset = req->requested;
    }

    g_SaveCounter++;
}
