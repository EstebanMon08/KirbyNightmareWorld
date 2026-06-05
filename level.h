#pragma once

/* ══════════════════════════════════════════════════════
   level.h — Estructuras de nivel y funciones de dibujo
   ══════════════════════════════════════════════════════ */

/* ── Dimensiones de piezas ── */
#define BLOCK_W 32
#define BLOCK_H 32
#define PLAT_W  45
#define PLAT_H   5
#define GOAL_W   42
#define GOAL_H   20

#define WORLD_W 1400

/* ── Estructuras ── */
struct GroundBlock { float x, y; };
struct Platform    { float x, y; };
struct Goal        { float x, y; bool active; };

extern const int MAX_GROUND;
extern const int MAX_PLATS;

extern GroundBlock ground   [];
extern Platform    platforms[];
extern Goal        goal;
extern int         numGround;
extern int         numPlats;
extern float       GROUND_Y;

/* ── Funciones ── */
void initLevel();

void drawBlock   (int screenX, int topY);
void drawPlatform(int screenX, int topY);
void drawGoal    (int screenX, int topY);
