#include "../include/actions.h"
#include <stdlib.h>
#include <time.h>

/* player 3 : 60% dash, 30% move, 10% teleport. */

static int rng_initialized = 0;

static void ensure_rng_initialized(void)
{
    if (!rng_initialized) {
        srand((unsigned int)time(NULL) ^ 0xCAFE0003);
        rng_initialized = 1;
    }
}

static char random_dash(void)
{
    static const char dashes[] = {
        ACTION_DASH_L, ACTION_DASH_R,
        ACTION_DASH_U, ACTION_DASH_D
    };
    return dashes[rand() % (int)(sizeof(dashes) / sizeof(dashes[0]))];
}

static char random_move(void)
{
    static const char moves[] = {
        ACTION_MOVE_L, ACTION_MOVE_R,
        ACTION_MOVE_U, ACTION_MOVE_D
    };
    return moves[rand() % (int)(sizeof(moves) / sizeof(moves[0]))];
}

static char random_teleport(void)
{
    static const char teleports[] = {
        ACTION_TELEPORT_L, ACTION_TELEPORT_R,
        ACTION_TELEPORT_U, ACTION_TELEPORT_D
    };
    return teleports[rand() % (int)(sizeof(teleports) / sizeof(teleports[0]))];
}

static char random_special(void)
{
    static const char specials[] = {
        ACTION_BOMB, ACTION_FORK, ACTION_CLEAN, ACTION_MUTE, ACTION_SWAP
    };
    return specials[rand() % (int)(sizeof(specials) / sizeof(specials[0]))];
}

char get_action(void)
{
    int roll;

    ensure_rng_initialized();
    roll = rand() % 100;

    if (roll < 55)
        return random_dash();
    if (roll < 80)
        return random_move();
    if (roll < 90)
        return random_teleport();
    return random_special();
}
