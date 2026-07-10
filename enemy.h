#pragma once
#include <stdbool.h>

/* ══════════════════════════════════════════════════════
   enemy.h — Tipos, structs y funciones de enemigos
   ══════════════════════════════════════════════════════ */

/* ── Dimensiones (también en sprites.h; se puede incluir uno u otro) ── */
#ifndef ENEMY_H
#define ENEMY_H 21
#endif
#ifndef ENEMY_W
#define ENEMY_W 43
#endif

/* ── Tipos de enemigo ── */
enum EnemyType {
    ENEMY_CUBE    = 0,
    ENEMY_SHOOTER = 1,
    ENEMY_BEAM    = 2,
    ENEMY_SWORD   = 3
};

/* ── Constantes de comportamiento ── */
#define CUBE_DIR_FRAMES    150
#define SHOOTER_DIR_FRAMES  90
#define BEAM_DIR_FRAMES     90
#define SWORD_DIR_FRAMES   150

#define FLAME_SPEED        2.8f
#define FLAME_LIFE         28
#define FLAME_BURST_COUNT  12
#define FLAME_BURST_GAP     2

#define BEAM_RADIUS      20.0f
#define BEAM_LIFE          22
#define BEAM_BURST_COUNT    9
#define BEAM_BURST_GAP      1

#define SWORD_SLASH_OFFSET  6
#define SWORD_SLASH_LIFE   16
#define SWORD_BURST_COUNT  16
#define SWORD_BURST_GAP     0

/* ── Struct Enemy ── */
struct Enemy {
    float x, y;
    int   w, h;
    bool  alive;
    int   dir;
    float speed;
    int   dirTimer;
    int   type;
    int   burstLeft;
    int   burstDelay;
    int   burstDir;
};

/* ── Struct EnemyShot ── */
struct EnemyShot {
    float x, y;
    float vx, vy;
    int   dir;
    bool  active;
    int   life;
    int   life0;
    int   shotType;  /* ENEMY_SWORD para slash, -1 para el resto */
};

extern const int NUM_ENEMIES;
extern Enemy     enemies[];

extern const int MAX_ENEMY_SHOTS;
extern EnemyShot enemyShots[];

/* ── Funciones ── */
void  initEnemies();
void  spawnEnemyShot(float x, float y, int dir, float vx, float vy, int life, int shotType = -1);
void* enemyThread(void*);
