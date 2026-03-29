#include "../include/actions.h"
#include <stdlib.h>
#include <time.h>

/* player 4 : keeps a direction for several turns. */

static int rng_initialized = 0;
static int behavior_initialized = 0;
static char current_action = ACTION_MOVE_R;
static int step_count = 0;
static int segment_length = 10;

static void ensure_rng_initialized(void)
{
    if (!rng_initialized) {
        srand((unsigned int)time(NULL) ^ 0xF00D0004);
        rng_initialized = 1;
    }
}

static char random_move(void)
{
    static const char dirs[] = {
        ACTION_MOVE_L, ACTION_MOVE_R, ACTION_MOVE_U, ACTION_MOVE_D
    };
    return dirs[rand() % (int)(sizeof(dirs) / sizeof(dirs[0]))];
}

static char random_dash(void)
{
    static const char dashes[] = {
        ACTION_DASH_L, ACTION_DASH_R,
        ACTION_DASH_U, ACTION_DASH_D
    };
    return dashes[rand() % (int)(sizeof(dashes) / sizeof(dashes[0]))];
}

static char random_teleport(void)
{
    static const char teleports[] = {
        ACTION_TELEPORT_L, ACTION_TELEPORT_R,
        ACTION_TELEPORT_U, ACTION_TELEPORT_D
    };
    return teleports[rand() % (int)(sizeof(teleports) / sizeof(teleports[0]))];
}

char get_action(void)
{
    int special_roll;

    ensure_rng_initialized();

    if (!behavior_initialized) {
        behavior_initialized = 1;
        current_action = random_move();
        segment_length = 5 + rand() % 16;
    }

    step_count++;

    if (step_count >= segment_length) {
        step_count = 0;
        segment_length = 5 + rand() % 16;

        special_roll = rand() % 10;
        if (special_roll == 0) {
            current_action = random_dash();
            return current_action;
        }
        if (special_roll == 1) {
            current_action = random_teleport();
            return current_action;
        }

        current_action = random_move();
    }

    return current_action;
}
