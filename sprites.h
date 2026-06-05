#pragma once
#include <wchar.h>

/* ══════════════════════════════════════════════════════
   sprites.h — Dimensiones y declaraciones de sprites
   ══════════════════════════════════════════════════════ */

/* ── Dimensiones de Kirby ── */
#define KIRBY_H  21
#define KIRBY_W  45

/* ── Dimensiones de enemigos ── */
#define ENEMY_H  21
#define ENEMY_W  43

/* ── Dimensiones del proyectil ── */
#define PROJ_H    9
#define PROJ_W   17

/* ── Dimensiones de los iconos de poder (HUD) ── */
#define ICON_H   22
#define ICON_W   35

/* ── Dimensiones de la meta ── */
#define GOAL_W    6
#define GOAL_H    4

/* ── Dimensiones del menú ── */
#define MENU_H   18
#define MENU_W   67

/* ── Sprites de Kirby ── */
extern const wchar_t* SPRITE_IDLE   [];
extern const wchar_t* SPRITE_WALK   [];
extern const wchar_t* SPRITE_JUMP   [];
extern const wchar_t* SPRITE_FALL   [];
extern const wchar_t* SPRITE_INHALE [];
extern const wchar_t* SPRITE_FULL   [];

/* ── Sprites de enemigos ── */
extern const wchar_t* SPRITE_ENEMY   [];
extern const wchar_t* SPRITE_SHOOTER [];
extern const wchar_t* SPRITE_BEAM    [];
extern const wchar_t* SPRITE_SWORD   [];

/* ── Proyectil universal ── */
extern const wchar_t* SPRITE_PROJECTILE[];

/* ── Iconos de poder (HUD) ── */
extern const wchar_t* ICON_NONE  [];
extern const wchar_t* ICON_FIRE  [];
extern const wchar_t* ICON_BEAM  [];
extern const wchar_t* ICON_SWORD [];

/* ── Meta y menú ── */
extern const wchar_t* SPRITE_GOAL [];
extern const wchar_t* SPRITE_MENU [];

/* ── Utilidades de espejado ── */
wchar_t mirrorChar(wchar_t c);
void    flipLine(const wchar_t* src, wchar_t* dst, int maxLen);
