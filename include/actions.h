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

/* action costs */
#define COST_MOVE       1
#define COST_DASH       10
#define COST_TELEPORT   2
#define COST_STILL      1

/*  constantes */
#define GRID_SIZE       100
#define INITIAL_CREDITS 9000
#define MAX_PLAYERS     4
#define DASH_DISTANCE   8
#define TELEPORT_DISTANCE 8

#endif /* ACTIONS_H */
