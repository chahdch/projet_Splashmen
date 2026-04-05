#ifndef ACTIONS_H
#define ACTIONS_H

/* Movement actions: 1 credit each */
#define ACTION_MOVE_L   1
#define ACTION_MOVE_R   2
#define ACTION_MOVE_U   3
#define ACTION_MOVE_D   4

/* no action: 1 credit */
#define ACTION_STILL    0

/* teleport actions: 2 credits each (teleports 8 cells) */
#define ACTION_TELEPORT_L   9
#define ACTION_TELEPORT_R   10
#define ACTION_TELEPORT_U   11
#define ACTION_TELEPORT_D   12

/* dash actions: 10 credits each (moves 8 cells) */
#define ACTION_DASH_L   5
#define ACTION_DASH_R   6
#define ACTION_DASH_U   7
#define ACTION_DASH_D   8

/* special actions */
#define ACTION_BOMB        13
#define ACTION_FORK        14
#define ACTION_CLEAN       15
#define ACTION_MUTE        16
#define ACTION_SWAP        17
#define ACTION_NUMBER      18

/* action costs */
#define COST_MOVE       1
#define COST_DASH       10
#define COST_TELEPORT   2
#define COST_STILL      1
#define COST_BOMB       9
#define COST_FORK       20
#define COST_CLEAN      40
#define COST_MUTE       30
#define COST_SWAP       35

/* effect durations */
#define BOMB_TIMER          5
#define BOMB_AREA_HALF      1
#define FORK_SPAWN_DELAY    5
#define FORK_DURATION      20
#define CLEAN_SIZE         7
#define MUTE_DURATION      10
#define SWAP_DURATION       5

/*  constantes */
#define GRID_SIZE       50
#define INITIAL_CREDITS 9000
#define MAX_PLAYERS     4
#define MAX_BOMBS       16
#define DASH_DISTANCE   8
#define TELEPORT_DISTANCE 8

#endif /* ACTIONS_H */
