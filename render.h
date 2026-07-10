#pragma once

/* ══════════════════════════════════════════════════════
   render.h — Funciones del hilo de render y director
   ══════════════════════════════════════════════════════ */

void  drawProjectile(int cxScreen, int cyScreen, int shotType = -1, int shotDir = 1);

void* directorThread (void*);
void* renderThread   (void*);
