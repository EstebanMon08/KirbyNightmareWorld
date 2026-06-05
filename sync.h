#pragma once
#include <pthread.h>
#include <semaphore.h>

/* ══════════════════════════════════════════════════════
   sync.h — Variables globales de sincronización
   Orden de adquisición de mutexes (NUNCA invertir):
       gMutex -> playerMutex -> enemyMutex -> projMutex -> ncMutex
   ══════════════════════════════════════════════════════ */

extern int SCREEN_W;
extern int SCREEN_H;
extern int currentLevel;

extern pthread_mutex_t gMutex;
extern pthread_mutex_t playerMutex;
extern pthread_mutex_t enemyMutex;
extern pthread_mutex_t projMutex;
extern pthread_mutex_t ncMutex;

extern pthread_cond_t  gModeCond;

extern sem_t semMouthful;
extern sem_t semSpit;
