#include <locale.h>
#include <ncurses.h>
#include <wchar.h>

#include "sync.h"
#include "sprites.h"
#include "level.h"

/* ══════════════════════════════════════════════════════
   level.cpp — Diseño de niveles, terreno y texturas
   Kirby Dreamland (ncurses)
   ══════════════════════════════════════════════════════ */

/* ── Datos de nivel ── */
const int   MAX_GROUND = 128;
const int   MAX_PLATS  = 64;

GroundBlock ground   [128];
Platform    platforms[64];
Goal        goal      = { 0, 0, false };
int         numGround = 0;
int         numPlats  = 0;
float       GROUND_Y  = 0;

/* ── Texturas (generadas una sola vez en buildTextures) ── */
static wchar_t groundTex[BLOCK_H][BLOCK_W + 1];
static wchar_t platTex  [PLAT_H ][PLAT_W  + 1];


/* ════════════════════════════════
   buildTextures
   ════════════════════════════════ */
static void buildTextures() {
    for (int r = 0; r < BLOCK_H; r++) {
        for (int c = 0; c < BLOCK_W; c++) {
            wchar_t ch;
            if (r == 0) ch = L'█';
            else {
                int cell = (c / 8 + (r - 1) / 8) % 2;
                ch = cell ? L'▓' : L'▒';
            }
            groundTex[r][c] = ch;
        }
        groundTex[r][BLOCK_W] = L'\0';
    }

    for (int r = 0; r < PLAT_H; r++) {
        for (int c = 0; c < PLAT_W; c++) {
            bool top = (r == 0),         bot = (r == PLAT_H - 1);
            bool lft = (c == 0),         rgt = (c == PLAT_W - 1);
            wchar_t ch;
            if      (top && lft) ch = L'▛';
            else if (top && rgt) ch = L'▜';
            else if (bot && lft) ch = L'▙';
            else if (bot && rgt) ch = L'▟';
            else if (top)        ch = L'▀';
            else if (bot)        ch = L'▄';
            else if (lft || rgt) ch = L'█';
            else                 ch = L'░';
            platTex[r][c] = ch;
        }
        platTex[r][PLAT_W] = L'\0';
    }
}

static void addGround  (float x, float y) { if (numGround < MAX_GROUND) ground    [numGround++] = { x, y }; }
static void addPlatform(float x, float y) { if (numPlats  < MAX_PLATS ) platforms [numPlats++ ] = { x, y }; }


/* ════════════════════════════════
   initLevel
   ════════════════════════════════ */
void initLevel() {
    buildTextures();
    GROUND_Y    = (float)(SCREEN_H - 3);
    numGround   = 0;
    numPlats    = 0;
    goal.active = false;

    if (currentLevel == 1) {
        for (float x = 0; x < WORLD_W; x += BLOCK_W)
            addGround(x, GROUND_Y);

        addPlatform(  90, GROUND_Y - 28);
        addPlatform( 260, GROUND_Y - 48);
        addPlatform( 380, GROUND_Y - 30);
        addPlatform( 680, GROUND_Y - 42);
        addPlatform( 820, GROUND_Y - 60);
        addPlatform(1010, GROUND_Y - 36);
        addPlatform(1120, GROUND_Y - 48);

        goal = { (float)(WORLD_W - 80), GROUND_Y - GOAL_H, true };
    }
    else {
        for (float x = 0; x < 128; x += BLOCK_W)
            addGround(x, GROUND_Y);

        addPlatform( 170, GROUND_Y -  28);
        addPlatform( 275, GROUND_Y -  40);
        addPlatform( 320, GROUND_Y -  40);
        addPlatform( 485, GROUND_Y -  30);
        addPlatform( 590, GROUND_Y -  24);
        addPlatform( 695, GROUND_Y -  48);
        addPlatform( 740, GROUND_Y -  48);
        addPlatform( 905, GROUND_Y -  60);
        addPlatform(1010, GROUND_Y -   8);
        addPlatform(1115, GROUND_Y -  24);
        addPlatform(1160, GROUND_Y -  24);
        addPlatform(1325, GROUND_Y -  10);

        goal = { 1325.0f + (PLAT_W - GOAL_W) / 2.0f,
                 GROUND_Y - 10 - GOAL_H,
                 true };
    }
}


/* ════════════════════════════════
   Dibujo de piezas (llamar con ncMutex tomado)
   ════════════════════════════════ */
static void blitRow(int row, int screenX, const wchar_t* tex, int texW) {
    if (row < 2 || row >= SCREEN_H) return;
    int start = 0, end = texW;
    if (screenX < 0)              start = -screenX;
    if (screenX + end > SCREEN_W)  end   = SCREEN_W - screenX;
    if (start >= end) return;

    wchar_t buf[200];
    int n = 0;
    for (int c = start; c < end && n < 199; c++) buf[n++] = tex[c];
    buf[n] = L'\0';
    mvaddwstr(row, screenX + start, buf);
}

void drawBlock(int screenX, int topY) {
    for (int r = 0; r < BLOCK_H; r++)
        blitRow(topY + r, screenX, groundTex[r], BLOCK_W);
}

void drawPlatform(int screenX, int topY) {
    for (int r = 0; r < PLAT_H; r++)
        blitRow(topY + r, screenX, platTex[r], PLAT_W);
}

void drawGoal(int screenX, int topY) {
    for (int r = 0; r < GOAL_H; r++)
        blitRow(topY + r, screenX, SPRITE_GOAL[r], GOAL_W);
}
