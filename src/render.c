#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"

/* ──────────────────────────────────────────────
   Minimal software renderer: draws into gs->pixels
   then blits with XPutImage.
   ────────────────────────────────────────────── */

#define PX(x, y) gs->pixels[(y) * WINDOW_WIDTH + (x)]
#define RGB(r,g,b) (((unsigned int)(r) << 16) | ((unsigned int)(g) << 8) | (unsigned int)(b))

static void fill_rect(GameState *gs, int x, int y, int w, int h, unsigned int color)
{
    for (int row = y; row < y + h; row++) {
        if (row < 0 || row >= WINDOW_HEIGHT) continue;
        for (int col = x; col < x + w; col++) {
            if (col < 0 || col >= WINDOW_WIDTH) continue;
            PX(col, row) = color;
        }
    }
}

static void draw_rect_outline(GameState *gs, int x, int y, int w, int h, unsigned int color)
{
    fill_rect(gs, x,       y,       w, 1, color);
    fill_rect(gs, x,       y+h-1,   w, 1, color);
    fill_rect(gs, x,       y,       1, h, color);
    fill_rect(gs, x+w-1,   y,       1, h, color);
}

/* ──────────────── 5×7 bitmap font ──────────────── */
/* Characters supported: 0-9 space : % P l a y e r W I N E R C ! c d s G O V T k */
static const unsigned char FONT[][7] = {
    /* 0 '0' */ {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E},
    /* 1 '1' */ {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E},
    /* 2 '2' */ {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F},
    /* 3 '3' */ {0x1F,0x02,0x04,0x02,0x01,0x11,0x0E},
    /* 4 '4' */ {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02},
    /* 5 '5' */ {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E},
    /* 6 '6' */ {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E},
    /* 7 '7' */ {0x1F,0x01,0x02,0x04,0x08,0x08,0x08},
    /* 8 '8' */ {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E},
    /* 9 '9' */ {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C},
    /* 10 ' ' */ {0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    /* 11 ':' */ {0x00,0x04,0x04,0x00,0x04,0x04,0x00},
    /* 12 '%' */ {0x11,0x0A,0x04,0x0E,0x11,0x0A,0x00},
    /* 13 'S' */ {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E},
    /* 14 'P' */ {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10},
    /* 15 'L' */ {0x10,0x10,0x10,0x10,0x10,0x10,0x1F},
    /* 16 'A' */ {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11},
    /* 17 'H' */ {0x11,0x11,0x11,0x1F,0x11,0x11,0x11},
    /* 18 'M' */ {0x11,0x1B,0x15,0x11,0x11,0x11,0x11},
    /* 19 'E' */ {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F},
    /* 20 'W' */ {0x11,0x11,0x15,0x15,0x0A,0x0A,0x00},
    /* 21 'I' */ {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E},
    /* 22 'N' */ {0x11,0x19,0x15,0x13,0x11,0x11,0x11},
    /* 23 'R' */ {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11},
    /* 24 'C' */ {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E},
    /* 25 '!' */ {0x04,0x04,0x04,0x04,0x04,0x00,0x04},
    /* 26 'G' */ {0x0E,0x11,0x10,0x17,0x11,0x11,0x0F},
    /* 27 'O' */ {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E},
    /* 28 'V' */ {0x11,0x11,0x11,0x11,0x0A,0x0A,0x04},
    /* 29 'T' */ {0x1F,0x04,0x04,0x04,0x04,0x04,0x04},
    /* 30 'c' */ {0x00,0x00,0x0E,0x10,0x10,0x11,0x0E},
    /* 31 'e' */ {0x00,0x00,0x0E,0x11,0x1F,0x10,0x0E},
    /* 32 'l' */ {0x0C,0x04,0x04,0x04,0x04,0x04,0x0E},
    /* 33 's' */ {0x00,0x00,0x0E,0x10,0x0E,0x01,0x1E},
    /* 34 'r' */ {0x00,0x00,0x16,0x19,0x10,0x10,0x10},
    /* 35 'a' */ {0x00,0x00,0x0E,0x01,0x0F,0x11,0x0F},
    /* 36 'y' */ {0x00,0x11,0x11,0x0F,0x01,0x01,0x0E},
    /* 37 'd' */ {0x01,0x01,0x0F,0x11,0x11,0x11,0x0F},
    /* 38 'i' */ {0x00,0x04,0x00,0x04,0x04,0x04,0x0E},
    /* 39 'n' */ {0x00,0x00,0x16,0x19,0x11,0x11,0x11},
    /* 40 'w' */ {0x00,0x11,0x11,0x15,0x15,0x0A,0x00},
    /* 41 'p' */ {0x00,0x1E,0x11,0x11,0x1E,0x10,0x10},
    /* 42 'k' */ {0x10,0x10,0x12,0x14,0x18,0x14,0x12},
    /* 43 'F' */ {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10},
    /* 44 'f' */ {0x06,0x08,0x1C,0x08,0x08,0x08,0x08},
    /* 45 'o' */ {0x00,0x00,0x0E,0x11,0x11,0x11,0x0E},
    /* 46 'm' */ {0x00,0x00,0x1B,0x15,0x15,0x15,0x15},
    /* 47 'v' */ {0x00,0x11,0x11,0x11,0x0A,0x04,0x00},
    /* 48 'u' */ {0x00,0x11,0x11,0x11,0x11,0x11,0x0F},
    /* 49 'x' */ {0x00,0x11,0x0A,0x04,0x0A,0x11,0x00},
    /* 50 't' */ {0x04,0x04,0x1F,0x04,0x04,0x04,0x03},
    /* 51 'g' */ {0x00,0x0F,0x11,0x11,0x0F,0x01,0x0E},
    /* 52 'h' */ {0x10,0x10,0x16,0x19,0x11,0x11,0x11},
    /* 53 'b' */ {0x10,0x10,0x1E,0x11,0x11,0x11,0x1E},
    /* 54 'z' */ {0x00,0x00,0x1F,0x02,0x04,0x08,0x1F},
    /* 55 'q' */ {0x00,0x0F,0x11,0x11,0x0F,0x01,0x00},
    /* 56 'j' */ {0x00,0x02,0x00,0x02,0x02,0x12,0x0C},
    /* 57 'D' */ {0x1C,0x12,0x11,0x11,0x11,0x12,0x1C},
    /* 58 'B' */ {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E},
    /* 59 'K' */ {0x11,0x12,0x14,0x18,0x14,0x12,0x11},
    /* 60 'U' */ {0x11,0x11,0x11,0x11,0x11,0x11,0x0E},
    /* 61 'X' */ {0x11,0x0A,0x04,0x04,0x04,0x0A,0x11},
    /* 62 'Y' */ {0x11,0x11,0x0A,0x04,0x04,0x04,0x04},
    /* 63 'Z' */ {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F},
    /* 64 'J' */ {0x07,0x02,0x02,0x02,0x02,0x12,0x0C},
    /* 65 'Q' */ {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D},
    /* 66 'F' dup */ {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10},
};

static int char_idx(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    switch (c) {
        case ' ': return 10; case ':': return 11; case '%': return 12;
        case 'S': return 13; case 'P': return 14; case 'L': return 15;
        case 'A': return 16; case 'H': return 17; case 'M': return 18;
        case 'E': return 19; case 'W': return 20; case 'I': return 21;
        case 'N': return 22; case 'R': return 23; case 'C': return 24;
        case '!': return 25; case 'G': return 26; case 'O': return 27;
        case 'V': return 28; case 'T': return 29;
        case 'c': return 30; case 'e': return 31; case 'l': return 32;
        case 's': return 33; case 'r': return 34; case 'a': return 35;
        case 'y': return 36; case 'd': return 37; case 'i': return 38;
        case 'n': return 39; case 'w': return 40; case 'p': return 41;
        case 'k': return 42; case 'F': return 43; case 'f': return 44;
        case 'o': return 45; case 'm': return 46; case 'v': return 47;
        case 'u': return 48; case 'x': return 49; case 't': return 50;
        case 'g': return 51; case 'h': return 52; case 'b': return 53;
        case 'z': return 54; case 'q': return 55; case 'j': return 56;
        case 'D': return 57; case 'B': return 58; case 'K': return 59;
        case 'U': return 60; case 'X': return 61; case 'Y': return 62;
        case 'Z': return 63; case 'J': return 64; case 'Q': return 65;
    }
    return 10;
}

static void draw_char(GameState *gs, char c, int x, int y, int scale, unsigned int color)
{
    int idx = char_idx(c);
    for (int row = 0; row < 7; row++) {
        for (int col = 0; col < 5; col++) {
            if (FONT[idx][row] & (0x10 >> col)) {
                fill_rect(gs, x + col * scale, y + row * scale, scale, scale, color);
            }
        }
    }
}

static void draw_text(GameState *gs, const char *str, int x, int y, int scale, unsigned int color)
{
    int cx = x;
    while (*str) {
        draw_char(gs, *str, cx, y, scale, color);
        cx += (5 + 1) * scale;
        str++;
    }
}

static int text_width(const char *s, int scale) { return (int)strlen(s) * 6 * scale; }

static void draw_bar(GameState *gs, int x, int y, int w, int h,
                     int val, int max_val, unsigned int color)
{
    fill_rect(gs, x, y, w, h, RGB(30, 30, 50));
    if (max_val > 0) {
        int filled = w * val / max_val;
        if (filled > 0) fill_rect(gs, x, y, filled, h, color);
    }
    draw_rect_outline(gs, x, y, w, h, RGB(70, 70, 90));
}

static const char *action_name(char action)
{
    switch ((unsigned char)action) {
        case 0: return "STILL";
        case 1: return "MOVE_L";
        case 2: return "MOVE_R";
        case 3: return "MOVE_U";
        case 4: return "MOVE_D";
        case 5: return "DASH_L";
        case 6: return "DASH_R";
        case 7: return "DASH_U";
        case 8: return "DASH_D";
        case 9: return "TELE_L";
        case 10: return "TELE_R";
        case 11: return "TELE_U";
        case 12: return "TELE_D";
        case 13: return "BOMB";
        case 14: return "FORK";
        case 15: return "CLEAN";
        case 16: return "MUTE";
        case 17: return "SWAP";
        default: return "?";
    }
}

static int find_winner(GameState *gs)
{
    int w = 0;
    for (int i = 1; i < gs->num_players; i++)
        if (gs->players[i].cells_owned > gs->players[w].cells_owned) w = i;
    return w;
}

void game_render(GameState *gs)
{
    if (!gs->pixels) return;
    /* Clear */
    memset(gs->pixels, 0x10, WINDOW_WIDTH * WINDOW_HEIGHT * sizeof(unsigned int));

    /* ── Title ── */
    draw_text(gs, "SPLASHMEM", GRID_OFFSET_X, 14, 3, RGB(180, 180, 255));

    /* Tick */
    char tbuf[32];
    snprintf(tbuf, sizeof(tbuf), "T%d", gs->tick);
    int tw = text_width(tbuf, 2);
    draw_text(gs, tbuf, WINDOW_WIDTH - GRID_OFFSET_X - tw, 18, 2, RGB(70, 70, 110));

    /* ── Grid ── */
    for (int gy = 0; gy < GRID_SIZE; gy++) {
        for (int gx = 0; gx < GRID_SIZE; gx++) {
            int owner = gs->grid[gy][gx].owner;
            int px = GRID_OFFSET_X + gx * CELL_SIZE;
            int py = GRID_OFFSET_Y + gy * CELL_SIZE;
            unsigned int color;
            if (owner == -1) {
                color = RGB(22, 22, 35);
            } else {
                Color3 c = gs->players[owner].color;
                if (!gs->players[owner].active) {
                    color = RGB(c.r / 3, c.g / 3, c.b / 3);
                } else {
                    color = RGB(c.r, c.g, c.b);
                }
            }
            fill_rect(gs, px, py, CELL_SIZE - 1, CELL_SIZE - 1, color);
        }
    }
    draw_rect_outline(gs, GRID_OFFSET_X - 1, GRID_OFFSET_Y - 1,
        GRID_SIZE * CELL_SIZE + 2, GRID_SIZE * CELL_SIZE + 2, RGB(90, 90, 130));

    /* ── Player markers (diamond) ── */
    for (int i = 0; i < gs->num_players; i++) {
        Player *p = &gs->players[i];
        if (!p->active) continue;
        int cx = GRID_OFFSET_X + p->x * CELL_SIZE + CELL_SIZE / 2;
        int cy = GRID_OFFSET_Y + p->y * CELL_SIZE + CELL_SIZE / 2;
        int r = CELL_SIZE + 2;
        for (int dy = -r; dy <= r; dy++) {
            for (int dx = -r; dx <= r; dx++) {
                if (abs(dx) + abs(dy) <= r) {
                    int fx = cx + dx, fy = cy + dy;
                    if (fx >= 0 && fx < WINDOW_WIDTH && fy >= 0 && fy < WINDOW_HEIGHT) {
                        if (abs(dx) + abs(dy) <= r - 2)
                            PX(fx, fy) = RGB(p->color.r, p->color.g, p->color.b);
                        else
                            PX(fx, fy) = RGB(255, 255, 255);
                    }
                }
            }
        }
    }

    /* ── Clone markers (smaller diamond) ── */
    for (int i = 0; i < gs->num_players; i++) {
        Clone *c = &gs->clones[i];
        if (!c->active) continue;
        Color3 col = gs->players[c->owner].color;
        int cx = GRID_OFFSET_X + c->x * CELL_SIZE + CELL_SIZE / 2;
        int cy = GRID_OFFSET_Y + c->y * CELL_SIZE + CELL_SIZE / 2;
        int r = CELL_SIZE / 2;
        for (int dy = -r; dy <= r; dy++) {
            for (int dx = -r; dx <= r; dx++) {
                if (abs(dx) + abs(dy) <= r) {
                    int fx = cx + dx, fy = cy + dy;
                    if (fx >= 0 && fx < WINDOW_WIDTH && fy >= 0 && fy < WINDOW_HEIGHT) {
                        PX(fx, fy) = RGB(col.r, col.g, col.b);
                    }
                }
            }
        }
    }

    /* ── Scoreboard ── */
    int sb_y = GRID_OFFSET_Y + GRID_SIZE * CELL_SIZE + 14;
    int gap = 8;
    int panel_w = (WINDOW_WIDTH - GRID_OFFSET_X * 2 - gap * (gs->num_players - 1)) / gs->num_players;

    for (int i = 0; i < gs->num_players; i++) {
        Player *p = &gs->players[i];
        int px = GRID_OFFSET_X + i * (panel_w + gap);
        unsigned int pc = RGB(p->color.r, p->color.g, p->color.b);

        fill_rect(gs, px, sb_y, panel_w, SCOREBOARD_H - 14, RGB(20, 20, 32));
        draw_rect_outline(gs, px, sb_y, panel_w, SCOREBOARD_H - 14, pc);

        /* Name */
        char lbl[16];
        snprintf(lbl, sizeof(lbl), "P%d", i + 1);
        draw_text(gs, lbl, px + 6, sb_y + 6, 2, pc);

        /* Status dot */
        unsigned int dot_c = p->active ? RGB(60, 220, 60) : RGB(180, 50, 50);
        fill_rect(gs, px + panel_w - 14, sb_y + 9, 8, 8, dot_c);

        /* Credits */
        char cbuf[32];
        snprintf(cbuf, sizeof(cbuf), "cr%d", p->credits);
        draw_text(gs, cbuf, px + 6, sb_y + 26, 1, RGB(180, 180, 180));
        draw_bar(gs, px + 6, sb_y + 37, panel_w - 12, 5, p->credits, INITIAL_CREDITS, pc);

        /* Action */
        const char *act = action_name(p->current_action);
        draw_text(gs, act, px + 6, sb_y + 48, 1, RGB(200, 200, 120));

        /* Cells */
        char cellbuf[32];
        snprintf(cellbuf, sizeof(cellbuf), "cells%d", p->cells_owned);
        draw_text(gs, cellbuf, px + 6, sb_y + 62, 1, RGB(180, 180, 180));
        draw_bar(gs, px + 6, sb_y + 73, panel_w - 12, 5,
                 p->cells_owned, GRID_SIZE * GRID_SIZE, pc);

        /* Pct */
        int pct = (p->cells_owned * 100) / (GRID_SIZE * GRID_SIZE);
        char pctbuf[16];
        snprintf(pctbuf, sizeof(pctbuf), "%d%%", pct);
        draw_text(gs, pctbuf, px + 6, sb_y + 86, 1, RGB(140, 140, 140));
    }

    /* ── Game Over overlay ── */
    if (gs->game_over) {
        /* Dim */
        for (int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; i++) {
            unsigned int p2 = gs->pixels[i];
            gs->pixels[i] = (((p2 & 0xFF0000) >> 1) & 0xFF0000) |
                             (((p2 & 0x00FF00) >> 1) & 0x00FF00) |
                             (((p2 & 0x0000FF) >> 1) & 0x0000FF);
        }

        int bw = 380, bh = 120;
        int bx = (WINDOW_WIDTH - bw) / 2;
        int by = (WINDOW_HEIGHT - bh) / 2;
        fill_rect(gs, bx, by, bw, bh, RGB(15, 15, 28));
        draw_rect_outline(gs, bx, by, bw, bh, RGB(220, 200, 50));
        draw_rect_outline(gs, bx+2, by+2, bw-4, bh-4, RGB(120, 100, 20));

        draw_text(gs, "GAME OVER", bx + 20, by + 14, 3, RGB(220, 200, 60));

        int win = find_winner(gs);
        Player *wp = &gs->players[win];
        char wbuf[32];
        snprintf(wbuf, sizeof(wbuf), "P%d WINS", win + 1);
        draw_text(gs, wbuf, bx + 20, by + 62, 3,
                  RGB(wp->color.r, wp->color.g, wp->color.b));

        char scbuf[20];
        snprintf(scbuf, sizeof(scbuf), "%d", wp->cells_owned);
        int sw = text_width(scbuf, 3);
        draw_text(gs, scbuf, bx + bw - sw - 20, by + 62, 3, RGB(210, 210, 210));
    }

    /* Controls display */
    int ctrl_y = 20;
    int ctrl_x = WINDOW_WIDTH - GRID_OFFSET_X - 180;
    draw_text(gs, "SPACE:Pause", ctrl_x, ctrl_y, 1, RGB(100, 200, 255));
    draw_text(gs, "R:Restart", ctrl_x, ctrl_y + 14, 1, RGB(100, 200, 255));
    draw_text(gs, "UP/DOWN:Speed", ctrl_x, ctrl_y + 28, 1, RGB(100, 200, 255));
    
    char speed_buf[32];
    snprintf(speed_buf, sizeof(speed_buf), "Speed:%dx", gs->speed_factor);
    draw_text(gs, speed_buf, ctrl_x, ctrl_y + 42, 1, RGB(100, 200, 255));

    if (gs->paused) {
        int pause_x = (WINDOW_WIDTH - text_width("[PAUSED]", 2)) / 2;
        draw_text(gs, "[PAUSED]", pause_x, ctrl_y + 60, 2, RGB(255, 100, 100));
    }

    /* Blit to texture */
    SDL_UpdateTexture(gs->texture, NULL, gs->pixels, WINDOW_WIDTH * sizeof(unsigned int));
    
    /* Render */
    if (!gs->renderer) return;
    SDL_RenderClear(gs->renderer);
    SDL_RenderCopy(gs->renderer, gs->texture, NULL, NULL);
    SDL_RenderPresent(gs->renderer);
}
