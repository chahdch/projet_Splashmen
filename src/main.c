#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "game.h"

int main(int argc, char **argv)
{
    GameState gs;
    int running = 1;
    int delay_us = 8000; /* ~120 updates/sec */
    int restart = 0;

    if (argc < 2 || argc > 5) {
        fprintf(stderr, "Usage: %s p1.so|p1.txt [p2.so|p2.txt] [p3.so|p3.txt] [p4.so|p4.txt]\n", argv[0]);
        return 1;
    }

    do {
        if (game_init(&gs, argc, argv) != 0) {
            fprintf(stderr, "Failed to initialize game.\n");
            return 1;
        }

        restart = 0;
        while (running) {
            int poll_result = game_poll_events(&gs);
            if (poll_result == 0) {
                running = 0;
                break;
            }
            if (poll_result == 2) {
                restart = 1;
                break;
            }

            if (!gs.paused && !gs.game_over) {
                for (int i = 0; i < gs.speed_factor; i++)
                    game_update(&gs);
            }

            game_render(&gs);
            usleep(delay_us);
        }

        if (restart) {
            game_cleanup(&gs);
        }
    } while (restart);


    game_cleanup(&gs);
    return 0;
}
