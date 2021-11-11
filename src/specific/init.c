#include "specific/init.h"

#include "3dsystem/phd_math.h"
#include "game/game.h"
#include "game/text.h"
#include "global/lib.h"
#include "global/vars.h"
#include "specific/clock.h"
#include "specific/display.h"
#include "specific/file.h"
#include "specific/frontend.h"
#include "specific/hwr.h"
#include "specific/input.h"
#include "specific/memory.h"
#include "specific/smain.h"
#include "specific/sndpc.h"

#include <stdio.h>
#include <time.h>
#include <windows.h>

void S_InitialiseSystem()
{
    S_SeedRandom();

    Lib_Init();
    Text_Init();
    ClockInit();
    SoundIsActive = SoundInit();
    MusicInit();
    InputInit();
    FMVInit();

    CalculateWibbleTable();

    HWR_InitialiseHardware();
}

void S_ExitSystem(const char *message)
{
    while (Input.select) {
        S_UpdateInput();
    }
    S_Memory_Shutdown();
    HWR_ShutdownHardware();
    ShowFatalError(message);
}

void S_ExitSystemFmt(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    char message[150];
    vsnprintf(message, 150, fmt, va);
    va_end(va);
    S_ExitSystem(message);
}

void CalculateWibbleTable()
{
    for (int i = 0; i < WIBBLE_SIZE; i++) {
        PHD_ANGLE angle = (i * PHD_360) / WIBBLE_SIZE;
        WibbleTable[i] = phd_sin(angle) * MAX_WIBBLE >> W2V_SHIFT;
        ShadeTable[i] = phd_sin(angle) * MAX_SHADE >> W2V_SHIFT;
        RandTable[i] = (GetRandomDraw() >> 5) - 0x01FF;
    }
}

void S_SeedRandom()
{
    time_t lt = time(0);
    struct tm *tptr = localtime(&lt);
    SeedRandomControl(tptr->tm_sec + 57 * tptr->tm_min + 3543 * tptr->tm_hour);
    SeedRandomDraw(tptr->tm_sec + 43 * tptr->tm_min + 3477 * tptr->tm_hour);
}
