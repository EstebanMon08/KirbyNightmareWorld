#pragma once

/* ══════════════════════════════════════════════════════
   render.h — Funciones del hilo de render y director
   ══════════════════════════════════════════════════════ */

void  drawProjectile(int cxScreen, int cyScreen);

void* directorThread (void*);
void* renderThread   (void*);
