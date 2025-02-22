#include "game/traps/teeth_trap.h"

#include "game/collide.h"
#include "game/control.h"
#include "game/effects/blood.h"
#include "game/sphere.h"
#include "global/vars.h"

BITE_INFO g_Teeth1A = { -23, 0, -1718, 0 };
BITE_INFO g_Teeth1B = { 71, 0, -1718, 1 };
BITE_INFO g_Teeth2A = { -23, 10, -1718, 0 };
BITE_INFO g_Teeth2B = { 71, 10, -1718, 1 };
BITE_INFO g_Teeth3A = { -23, -10, -1718, 0 };
BITE_INFO g_Teeth3B = { 71, -10, -1718, 1 };

static void BaddieBiteEffect(ITEM_INFO *item, BITE_INFO *bite);

static void BaddieBiteEffect(ITEM_INFO *item, BITE_INFO *bite)
{
    PHD_VECTOR pos;
    pos.x = bite->x;
    pos.y = bite->y;
    pos.z = bite->z;
    GetJointAbsPosition(item, &pos, bite->mesh_num);
    DoBloodSplat(
        pos.x, pos.y, pos.z, item->speed, item->pos.y_rot, item->room_number);
}

void SetupTeethTrap(OBJECT_INFO *obj)
{
    obj->control = TeethTrapControl;
    obj->collision = TrapCollision;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

void TeethTrapControl(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    if (TriggerActive(item)) {
        item->goal_anim_state = TT_NASTY;
        if (item->touch_bits && item->current_anim_state == TT_NASTY) {
            g_LaraItem->hit_points -= TEETH_TRAP_DAMAGE;
            g_LaraItem->hit_status = 1;
            BaddieBiteEffect(item, &g_Teeth1A);
            BaddieBiteEffect(item, &g_Teeth1B);
            BaddieBiteEffect(item, &g_Teeth2A);
            BaddieBiteEffect(item, &g_Teeth2B);
            BaddieBiteEffect(item, &g_Teeth3A);
            BaddieBiteEffect(item, &g_Teeth3B);
        }
    } else {
        item->goal_anim_state = TT_NICE;
    }
    AnimateItem(item);
}
