#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "game.h"

int main(int argc, char **argv)
{
    GameState gs;
    int running = 1;
    int delay_us = 8000; /* ~120 updates/sec */

    if (argc < 2 || argc > 5) {
        fprintf(stderr, "Usage: %s p1.so [p2.so] [p3.so] [p4.so]\n", argv[0]);
        return 1;
    }

    if (game_init(&gs, argc, argv) != 0) {
        fprintf(stderr, "Failed to initialize game.\n");
        return 1;
    }

    while (running) {
        running = game_poll_events(&gs);

        if (!gs.game_over)
            game_update(&gs);

        game_render(&gs);
        usleep(delay_us);
    }

    game_cleanup(&gs);
    return 0;
}
