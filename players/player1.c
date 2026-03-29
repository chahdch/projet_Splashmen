#include "../include/actions.h"
#include <stdlib.h>
#include <time.h>

/*player 1 : picks uniformly among every available action. */

static int rng_initialized = 0;

static void ensure_rng_initialized(void)
{
    if (!rng_initialized) {
        srand((unsigned int)time(NULL) ^ 0xDEAD0001);
        rng_initialized = 1;
    }
}

char get_action(void)
{
    static const char actions[] = {
        ACTION_MOVE_L, ACTION_MOVE_R, ACTION_MOVE_U, ACTION_MOVE_D,
        ACTION_DASH_L, ACTION_DASH_R, ACTION_DASH_U, ACTION_DASH_D,
        ACTION_TELEPORT_L, ACTION_TELEPORT_R, ACTION_TELEPORT_U, ACTION_TELEPORT_D,
        ACTION_STILL
    };
    int total_actions;

    ensure_rng_initialized();
    total_actions = (int)(sizeof(actions) / sizeof(actions[0]));
    return actions[rand() % total_actions];
}
