#pragma once
#include <stdbool.h>

/* ══════════════════════════════════════════════════════
   projectile.h — Pools de proyectiles y sus hilos
   ══════════════════════════════════════════════════════ */

/* ── Struct Star (proyectil al escupir) ── */
struct Star {
    float x, y;
    int   dir;
    bool  active;
    float speed;
};

/* ── Struct KirbyShot (llamas / rayo / slash) ── */
struct KirbyShot {
    float x, y, vx, vy;
    bool  active;
    int   life;
    int   shotType;  /* ENEMY_SWORD para slash, -1 para el resto */
    int   dir;       /* 1 = derecha, -1 = izquierda */
};

extern Star          gStar;
extern const int     MAX_KIRBY_SHOTS;
extern KirbyShot     kirbyShots[];

/* ── Funciones ── */
void  spawnKirbyShot(float x, float y, float vx, float vy, int life, int shotType = -1, int dir = 1);

void* starThread      (void*);
void* kirbyShotThread (void*);
void* enemyShotThread (void*);
