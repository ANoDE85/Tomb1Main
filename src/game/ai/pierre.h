#ifndef T1M_GAME_AI_PIERRE_H
#define T1M_GAME_AI_PIERRE_H

#include "game/types.h"
#include <stdint.h>

#define PIERRE_POSE_CHANCE 0x60 // = 96
#define PIERRE_SHOT_DAMAGE 50
#define PIERRE_WALK_TURN (PHD_DEGREE * 3) // = 546
#define PIERRE_RUN_TURN (PHD_DEGREE * 6) // = 1092
#define PIERRE_WALK_RANGE SQUARE(WALL_L * 3) // = 9437184
#define PIERRE_DIE_ANIM 12
#define PIERRE_WIMP_CHANCE 0x2000
#define PIERRE_RUN_HITPOINTS 40
#define PIERRE_DISAPPEAR 10

typedef enum {
    PIERRE_EMPTY = 0,
    PIERRE_STOP = 1,
    PIERRE_WALK = 2,
    PIERRE_RUN = 3,
    PIERRE_AIM = 4,
    PIERRE_DEATH = 5,
    PIERRE_POSE = 6,
    PIERRE_SHOOT = 7,
} PIERRE_ANIM;

extern BITE_INFO PierreGun1;
extern BITE_INFO PierreGun2;

void SetupPierre(OBJECT_INFO *obj);
void PierreControl(int16_t item_num);

#endif
