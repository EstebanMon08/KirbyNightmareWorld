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
};

extern Star          gStar;
extern const int     MAX_KIRBY_SHOTS;
extern KirbyShot     kirbyShots[];

/* ── Funciones ── */
void  spawnKirbyShot(float x, float y, float vx, float vy, int life);

void* starThread      (void*);
void* kirbyShotThread (void*);
void* enemyShotThread (void*);
