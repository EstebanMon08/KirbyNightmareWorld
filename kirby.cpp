#include <locale.h>
#include <ncurses.h>
#include <wchar.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <semaphore.h>

#define FPS         30
#define WORLD_W    1000

/* Tamaño real de la terminal, detectado en runtime */
int SCREEN_W = 80;
int SCREEN_H = 24;

/* ── Sprites con caracteres especiales (wchar_t) ── */

#define KIRBY_H     21
#define KIRBY_W     45

/* Intercambia caracteres espejo al voltear un sprite */
wchar_t mirrorChar(wchar_t c) {
    switch (c) {
        case L'(': return L')';
        case L')': return L'(';
        case L'/': return L'\\';
        case L'\\': return L'/';
        case L'<': return L'>';
        case L'>': return L'<';
        case L'[': return L']';
        case L']': return L'[';
        case L'{': return L'}';
        case L'}': return L'{';
        default:   return c;
    }
}

/* Voltea una línea de sprite horizontalmente */
void flipLine(const wchar_t* src, wchar_t* dst, int maxLen) {
    int len = wcslen(src);
    if (len > maxLen - 1) len = maxLen - 1;
    for (int i = 0; i < len; i++) {
        dst[i] = mirrorChar(src[len - 1 - i]);
    }
    dst[len] = L'\0';
}

const wchar_t* SPRITE_IDLE[KIRBY_H] = {
   L"                                      ",
   L"                                      ",
   L"              ███████████             ",
   L"          ████           ████         ",
   L"       ███                   ███      ",
   L"     ██                         ██    ",
   L"    █                             █   ",
   L"    █                              █  ",
   L"  ██                 ▒▒█   ▒▒█     █  ",
   L" █                   ▒▒█   ▒▒█      █ ",
   L"█                    ███   ███       █",
   L"█                    ███   ███       █",
   L"█            ▒▒▒▒▒             ▒▒▒▒  █",
   L"█                                    █",
   L"█       █               ███       █  █",
   L" ██    █                         ██ █ ",
   L"   ████                         ████  ",
   L"       ███                   █████    ",
   L"    ███▒▒▒███████       █████▒▒▒▒██   ",
   L"  ██▒▒▒▒▒▒▒▒▒▒▒▒████████▒▒▒▒▒▒▒▒▒▒▒██ ",
   L"    █████████████      █████████████  "
};

const wchar_t* SPRITE_WALK[KIRBY_H] = {
   L"                 ███████████                 ",
   L"             ████           ████             ",
   L"          ███                   ███          ",
   L"        ██                         ██        ",
   L"     ██████                          ██      ",
   L"   ███                  ▒▒█   ▒▒█     ████   ",
   L" ███                    ▒▒█   ▒▒█     ██ ██  ",
   L"██                      ███   ███     ██   ██",
   L"██                      ███   ███     ██    █",
   L"██              ▒▒▒▒▒             ▒▒▒▒██    █",
   L" ██                                   ██   ██",
   L"   ███                     ███        ██  ██ ",
   L"     █████                           █████   ",
   L"       ██                           ██       ",
   L"      ████                         ████████  ",
   L"     ██▒▒████                   ████▒▒▒▒▒▒██ ",
   L"     ██▒▒▒▒▒▒███           █████▒▒▒▒▒▒▒▒▒▒██ ",
   L"      ██▒▒▒▒▒▒▒▒██████████████▒▒▒▒▒▒▒▒█████  ",
   L"       ███▒▒▒▒▒▒▒▒██         ██████████      ",
   L"         ████▒▒▒▒███                         ",
   L"            ██████                           ",
};

const wchar_t* SPRITE_JUMP[KIRBY_H] = {
   L"            █████████████            ",
   L"  ███████ ██             ██ ███████  ",
   L" █       █                 █      ██ ",
   L"█                                  ██",
   L"█                                   █",
   L" █                     ▒█  ▒█      ██",
   L"  ██                   ▒█  ▒█     ██ ",
   L"  █                    ██  ██     █  ",
   L" █              ▒▒▒           ▒▒▒  █ ",
   L" █                  █              █ ",
   L" █                   █████████     █ ",
   L" █                  █              █ ",
   L"  █                                █ ",
   L"  █                               █  ",
   L"   ██                            █   ",
   L"  ████                         ██    ",
   L" ██▒▒▒██                      █      ",
   L" █▒▒▒▒▒▒██████           █████       ",
   L" █▒▒▒▒▒▒█▒▒▒█ ███████████            ",
   L" ██▒▒▒██▒▒▒█                         ",
   L"   ███  ███                          ",
};

const wchar_t* SPRITE_FALL[KIRBY_H] = {
   L"            █████████████            ",
   L"  ███████ ██             ██ ███████  ",
   L" █       █                 █      ██ ",
   L"█                                  ██",
   L"█                                   █",
   L" █                     ▒█  ▒█      ██",
   L"  ██                   ▒█  ▒█     ██ ",
   L"  █                    ██  ██     █  ",
   L" █              ▒▒▒           ▒▒▒  █ ",
   L" █                  █              █ ",
   L" █                   █████████     █ ",
   L" █                  █              █ ",
   L"  █                                █ ",
   L"  █                               █  ",
   L"   ██                            █   ",
   L"  ████                         ██    ",
   L" ██▒▒▒██                      █      ",
   L" █▒▒▒▒▒▒██████           █████       ",
   L" █▒▒▒▒▒▒█▒▒▒█ ███████████            ",
   L" ██▒▒▒██▒▒▒█                         ",
   L"   ███  ███                          ",
};

const wchar_t* SPRITE_INHALE[KIRBY_H] = {
   L"  ████████  ██████████████  ██████  ",
   L" █        ██              ███    ███",
   L"█                  ▒     ▒  ███    █",
   L"█                   ▒█    ▒█   █   █",
   L" ██                  ███   ██   █ ██",
   L"  ███           ▒▒▒▒  ██   ██▒▒  ██ ",
   L" █                                █ ",
   L" █                       ████      █",
   L" █                      █▓▓▓▓█     █",
   L" █                     █▓▓▓▓▓▓█    █",
   L" █                    █▓▓▓▓▓▓▓█    █",
   L" █                    █▓▓▓▓▓▓▓█    █",
   L" █                    █▓▓▓▓▓▓▓█    █",
   L"  █                   █▓▓▓▓▓▓█    █ ",
   L"   █                   █▓▓▓▓█    █  ",
   L"  ███                   ████    █   ",
   L"  █▒▒███                     ███    ",
   L"  █▒▒▒▒▒████              ███▒▒▒█   ",
   L"  █▒▒▒▒▒▒▒▒███████████████▒▒▒▒▒▒██  ",
   L"  ██▒▒▒▒▒▒▒█             ██▒▒▒▒███  ",
   L"   ████████                █████    ",
};

/* ── Enemigos ── */

#define ENEMY_H 5
#define ENEMY_W 8

/* Tipos de enemigo */
enum EnemyType { ENEMY_CUBE = 0, ENEMY_SHOOTER = 1 };

/* Frames entre cambios de dirección según el tipo */
#define CUBE_DIR_FRAMES    150  /* el cubo recorre tramos largos (~5 s) */
#define SHOOTER_DIR_FRAMES  90  /* el lanzallamas se mueve poco y voltea seguido (~3 s) */

/* Parámetros del lanzallamas (enemigo de fuego) */
#define FLAME_SPEED        1.2f /* velocidad de cada llama */
#define FLAME_LIFE         14   /* frames de vida de cada llama antes de disiparse */
#define FLAME_BURST_COUNT  12   /* cantidad de llamas por ráfaga */
#define FLAME_BURST_GAP     2   /* frames entre una llama y la siguiente */

const wchar_t* SPRITE_ENEMY[ENEMY_H] = {
    L"████████",
    L"█ ▒  ▒ █",
    L"█      █",
    L"█ ▒▒▒▒ █",
    L"████████",
};

/* Sprite del lanzador: bloque más sólido (▓) con dos ojos, para distinguirlo */
const wchar_t* SPRITE_SHOOTER[ENEMY_H] = {
    L"████████",
    L"█▓▓▓▓▓▓█",
    L"█▓▒▓▓▒▓█",
    L"█▓▓▓▓▓▓█",
    L"████████",
};

struct Enemy {
    float x, y;
    int   w, h;
    bool  alive;
    int   dir;          /* 1 = derecha, -1 = izquierda */
    float speed;        /* velocidad de movimiento */
    int   dirTimer;     /* frames restantes antes de cambiar dirección */
    int   type;         /* ENEMY_CUBE o ENEMY_SHOOTER */
    int   burstLeft;    /* llamas que faltan por lanzar en la ráfaga actual */
    int   burstDelay;   /* frames hasta la siguiente llama de la ráfaga */
    int   burstDir;     /* dirección fija del chorro de fuego */
};

const int NUM_ENEMIES = 3;
Enemy enemies[NUM_ENEMIES];

/* ── Proyectiles de los enemigos (llamas del lanzallamas) ── */

struct EnemyShot {
    float x, y;
    int   dir;
    bool  active;
    float speed;
    int   life;        /* frames de vida restantes; al llegar a 0 la llama se disipa */
};
const int MAX_ENEMY_SHOTS = 32;  /* pool amplio: un chorro mantiene varias llamas vivas */
EnemyShot enemyShots[MAX_ENEMY_SHOTS];

/* Activa el primer proyectil libre del pool (llamar con gMutex tomado) */
void spawnEnemyShot(float x, float y, int dir) {
    for (int i = 0; i < MAX_ENEMY_SHOTS; i++) {
        if (!enemyShots[i].active) {
            enemyShots[i] = { x, y, dir, true, FLAME_SPEED, FLAME_LIFE };
            return;
        }
    }
}

void initEnemies() {
    int groundY = SCREEN_H - 3;
    float gy = (float)(groundY - ENEMY_H);
    enemies[0] = { 80,  gy, ENEMY_W, ENEMY_H, true,  1, 1.0f, CUBE_DIR_FRAMES,    ENEMY_CUBE,    0, 0, 0 };
    enemies[1] = { 230, gy, ENEMY_W, ENEMY_H, true, -1, 0.4f, SHOOTER_DIR_FRAMES, ENEMY_SHOOTER, 0, 0, 0 };
    enemies[2] = { 400, gy, ENEMY_W, ENEMY_H, true,  1, 1.0f, CUBE_DIR_FRAMES,    ENEMY_CUBE,    0, 0, 0 };

    /* Limpiar todos los proyectiles enemigos */
    for (int i = 0; i < MAX_ENEMY_SHOTS; i++) enemyShots[i].active = false;
}

/* ── Estrella (proyectil al escupir) ── */

struct Star {
    float x, y;
    int   dir;     /* 1 = derecha, -1 = izquierda */
    bool  active;
    float speed;
};
Star gStar = { 0, 0, 1, false, 4.0f };

/* ── Plataformas ── */

struct Platform { float x, y; int width; };
const int NUM_PLATFORMS = 8;
Platform platforms[NUM_PLATFORMS];

void initPlatforms() {
    int groundY = SCREEN_H - 3;
    platforms[0] = {  0,  (float)groundY, 180};
    platforms[1] = {200,  (float)groundY, 120};
    platforms[2] = {350,  (float)groundY, 250};
    platforms[3] = { 30,  (float)(groundY - 8),  25};
    platforms[4] = { 90,  (float)(groundY - 15), 20};
    platforms[5] = {160,  (float)(groundY - 20), 18};
    platforms[6] = {260,  (float)(groundY - 16), 22};
    platforms[7] = {380,  (float)(groundY - 10), 30};
}

/* ── Estado del juego ── */

#define INVINCIBLE_FRAMES 60  /* frames de invencibilidad tras recibir daño */

/* ── Succión ── */
#define INHALE_RANGE 28   /* alcance de la zona de succión frente a Kirby (columnas) */
#define INHALE_PULL  2.5f /* velocidad con que el enemigo es arrastrado a la boca */
#define MOUTHFUL_MAX 1    /* Kirby solo puede tener un enemigo en la boca a la vez */

struct GameState {
    float worldX, y, velY, cameraX;
    bool  onGround, jumping, running;
    bool  keyLeft, keyRight, keyJump, keyInhale;
    bool  keySwallow;      /* tecla de tragar (S) */
    int   jumpCount;
    bool  jumpHeld;
    bool  eHeld;           /* flanco de la tecla E (inhalar/disparar) */
    bool  swallowHeld;     /* flanco de la tecla tragar (S) */
    bool  inhaling;        /* true cuando está succionando */
    int   facingDir;       /* 1 = derecha, -1 = izquierda */
    int   hp, maxHp;
    int   invincibleTimer; /* frames restantes de invencibilidad */
    bool  gameOver;
    int   gameOverTimer;   /* frames restantes antes de reiniciar */
};

/* Mutex para estado del juego */
pthread_mutex_t gMutex = PTHREAD_MUTEX_INITIALIZER;
/* Mutex dedicado para ncurses */
pthread_mutex_t ncMutex = PTHREAD_MUTEX_INITIALIZER;

/* Semáforos:
   semMouthful — CONTADOR: enemigos tragados disponibles para escupir (munición).
                 Productor: physics al absorber (sem_post).
                 Consumidor: physics al escupir (sem_trywait). HUD lo lee con sem_getvalue.
   semSpit     — SEÑALIZACIÓN entre hilos: physics avisa "hay que lanzar estrella" (sem_post)
                 y el hilo de la estrella espera dormido (sem_wait) hasta que llega la señal. */
sem_t semMouthful;
sem_t semSpit;

GameState gState;

static int noLeftCount  = 99;
static int noRightCount = 99;
static int noJumpCount  = 99;
static int noInhaleCount = 99;
static int noSwallowCount = 99;
const  int RELEASE_THRESHOLD = 8;

/* Reiniciar el juego a su estado inicial */
void resetGame() {
    gState.worldX = 10; gState.y = platforms[0].y - KIRBY_H;
    gState.velY = 0; gState.cameraX = 0; gState.onGround = true;
    gState.jumping = false;
    gState.keyLeft = false; gState.keyRight = false; gState.keyJump = false;
    gState.keyInhale = false; gState.inhaling = false;
    gState.eHeld = false;
    gState.keySwallow = false; gState.swallowHeld = false;
    gState.jumpCount = 0; gState.jumpHeld = false;
    gState.facingDir = 1;
    gState.hp = gState.maxHp;
    gState.invincibleTimer = 0;
    gState.gameOver = false;
    gState.gameOverTimer = 0;
    /* Vaciar la boca: dejar el contador de munición en 0 */
    while (sem_trywait(&semMouthful) == 0) { /* drenar */ }
    gStar.active = false;
    /* Reiniciar enemigos */
    initEnemies();
}

/* ── Hilo de input ── */

void* inputThread(void*) {
    while (true) {
        pthread_mutex_lock(&gMutex);
        if (!gState.running) { pthread_mutex_unlock(&gMutex); break; }
        pthread_mutex_unlock(&gMutex);

        int ch;
        bool sawLeft = false, sawRight = false, sawJump = false, sawInhale = false;
        bool sawSwallow = false;

        pthread_mutex_lock(&ncMutex);
        while ((ch = getch()) != ERR) {
            switch (ch) {
                case 'a': case 'A': sawLeft  = true; break;
                case 'd': case 'D': sawRight = true; break;
                case 'w': case 'W': case ' ': sawJump = true; break;
                case 'e': case 'E': sawInhale = true; break;
                case 's': case 'S': sawSwallow = true; break;
                case 'q': case 'Q': case 27:
                    pthread_mutex_unlock(&ncMutex);
                    pthread_mutex_lock(&gMutex);
                    gState.running = false;
                    pthread_mutex_unlock(&gMutex);
                    /* Despertar al hilo de la estrella para que pueda terminar */
                    sem_post(&semSpit);
                    return nullptr;
            }
        }
        pthread_mutex_unlock(&ncMutex);

        if (sawLeft)   noLeftCount   = 0; else noLeftCount++;
        if (sawRight)  noRightCount  = 0; else noRightCount++;
        if (sawJump)   noJumpCount   = 0; else noJumpCount++;
        if (sawInhale) noInhaleCount = 0; else noInhaleCount++;
        if (sawSwallow) noSwallowCount = 0; else noSwallowCount++;

        pthread_mutex_lock(&gMutex);
        gState.keyLeft   = (noLeftCount   < RELEASE_THRESHOLD);
        gState.keyRight  = (noRightCount  < RELEASE_THRESHOLD);
        gState.keyJump   = (noJumpCount   < RELEASE_THRESHOLD);
        gState.keyInhale = (noInhaleCount < RELEASE_THRESHOLD);
        gState.keySwallow = (noSwallowCount < RELEASE_THRESHOLD);
        pthread_mutex_unlock(&gMutex);

        usleep(8000);
    }
    return nullptr;
}

/* ── Hilo de fisica ── */

void* physicsThread(void*) {
    while (true) {
        pthread_mutex_lock(&gMutex);
        if (!gState.running) { pthread_mutex_unlock(&gMutex); break; }

        /* Si estamos en game over, solo contar el timer y reiniciar */
        if (gState.gameOver) {
            gState.gameOverTimer--;
            if (gState.gameOverTimer <= 0) {
                resetGame();
            }
            pthread_mutex_unlock(&gMutex);
            usleep(33333);
            continue;
        }

        /* Estado actual de la boca (la cuenta vive en el semáforo) */
        int curMouth;
        sem_getvalue(&semMouthful, &curMouth);
        bool mouthFull = (curMouth > 0);

        /* Tecla E:
           - boca llena  -> dispara la estrella (flanco de E)
           - boca vacía  -> inhala
           La succión queda deshabilitada mientras haya un enemigo en la boca. */
        if (gState.keyInhale && !gState.eHeld && mouthFull) {
            if (sem_trywait(&semMouthful) == 0) {  /* sacar el enemigo de la boca */
                sem_post(&semSpit);                /* señalizar al hilo de la estrella */
            }
        }
        gState.eHeld = gState.keyInhale;

        /* Succión: solo con la boca vacía y en el suelo */
        gState.inhaling = gState.keyInhale && gState.onGround && !mouthFull;

        /* Tragar (flanco de tecla S): si hay un enemigo en la boca, digerirlo.
           Consume el enemigo pero NO lanza ninguna estrella. */
        if (gState.keySwallow && !gState.swallowHeld) {
            sem_trywait(&semMouthful);  /* vaciar la boca (si hay algo) */
            gState.swallowHeld = true;
        }
        if (!gState.keySwallow) gState.swallowHeld = false;

        /* Movimiento (bloqueado si está succionando) */
        if (!gState.inhaling) {
            if (gState.keyLeft)  { gState.worldX -= 2.0f; gState.facingDir = -1; }
            if (gState.keyRight) { gState.worldX += 2.0f; gState.facingDir =  1; }
        }
        if (gState.worldX < 0) gState.worldX = 0;
        if (gState.worldX > WORLD_W - KIRBY_W) gState.worldX = WORLD_W - KIRBY_W;

        /* Salto (bloqueado si está succionando) */
        if (gState.keyJump && !gState.jumpHeld && !gState.inhaling) {
            if (gState.onGround) {
                gState.velY = -3.0f; gState.onGround = false;
                gState.jumping = true; gState.jumpCount = 1;
            } else if (gState.jumpCount < 5) {
                gState.jumpCount++;
                switch (gState.jumpCount) {
                    case 2: gState.velY = -2.0f; break;
                    case 3: gState.velY = -2.0f; break;
                    case 4: gState.velY = -2.0f; break;
                    case 5: gState.velY = -2.0f; break;
                }
            }
            gState.jumpHeld = true;
        }
        if (!gState.keyJump) gState.jumpHeld = false;

        /* Gravedad */
        if (!gState.onGround) { gState.velY += 0.18f; gState.y += gState.velY; }

        /* Respawn si cae fuera de pantalla */
        if (gState.y > SCREEN_H + 5) {
            gState.y = platforms[0].y - KIRBY_H;
            gState.velY = 0; gState.onGround = true;
            gState.jumping = false; gState.jumpCount = 0;
        }

        /* Colisión con plataformas */
        gState.onGround = false;
        for (int i = 0; i < NUM_PLATFORMS; i++) {
            Platform& p = platforms[i];
            bool dentroX = (gState.worldX + KIRBY_W > p.x) && (gState.worldX < p.x + p.width);
            bool cayendo = gState.velY >= 0;
            bool sobreY  = (gState.y + KIRBY_H >= p.y) && (gState.y + KIRBY_H <= p.y + 3);
            if (dentroX && cayendo && sobreY) {
                gState.y = p.y - KIRBY_H; gState.velY = 0;
                gState.onGround = true; gState.jumping = false; gState.jumpCount = 0;
                break;
            }
        }

        /* ── Zona de succión frente a Kirby ──
           Solo activa mientras succiona. Se extiende INHALE_RANGE columnas
           desde la boca hacia la dirección en que mira. */
        bool  inhaleActive = gState.inhaling;
        float bodyLeft  = gState.worldX + 5;
        float bodyRight = gState.worldX + KIRBY_W - 5;
        float mouthX    = (gState.facingDir == 1) ? bodyRight : bodyLeft;
        float zoneY1    = gState.y + 3;
        float zoneY2    = gState.y + KIRBY_H - 3;
        float zoneX1, zoneX2;
        if (gState.facingDir == 1) {
            zoneX1 = bodyRight - 5;
            zoneX2 = bodyRight + INHALE_RANGE;
        } else {
            zoneX1 = bodyLeft - INHALE_RANGE;
            zoneX2 = bodyLeft + 5;
        }

        /* Reducir timer de invencibilidad */
        if (gState.invincibleTimer > 0) gState.invincibleTimer--;

        /* Movimiento, succión y colisión de enemigos */
        for (int i = 0; i < NUM_ENEMIES; i++) {
            if (!enemies[i].alive) continue;
            Enemy& e = enemies[i];

            float ex1 = e.x,       ey1 = e.y;
            float ex2 = e.x + e.w, ey2 = e.y + e.h;

            /* ¿El enemigo cae dentro de la zona de succión? */
            bool enZona = inhaleActive &&
                          (ex1 < zoneX2) && (ex2 > zoneX1) &&
                          (ey1 < zoneY2) && (ey2 > zoneY1);

            if (enZona) {
                /* Arrastrar al enemigo hacia la boca de Kirby */
                float enemyCenter = e.x + e.w / 2.0f;
                if (enemyCenter < mouthX) e.x += INHALE_PULL;
                else                      e.x -= INHALE_PULL;

                /* Si llegó a la boca y aún cabe en ella, queda absorbido */
                if (fabsf((e.x + e.w / 2.0f) - mouthX) < 6.0f) {
                    int cur;
                    sem_getvalue(&semMouthful, &cur);
                    if (cur < MOUTHFUL_MAX) {
                        e.alive = false;
                        sem_post(&semMouthful);  /* +1 munición disponible */
                    }
                    /* Si la boca está llena, el enemigo simplemente no se traga */
                }
                /* Mientras es succionado no se mueve solo ni hace daño */
                continue;
            }

            /* Movimiento normal del enemigo (el lanzallamas se detiene
               mientras escupe fuego) */
            bool bursting = (e.type == ENEMY_SHOOTER && e.burstLeft > 0);
            if (!bursting) {
                e.x += e.speed * e.dir;
                e.dirTimer--;
                if (e.dirTimer <= 0) {
                    /* El lanzallamas inicia una ráfaga de fuego hacia donde mira */
                    if (e.type == ENEMY_SHOOTER) {
                        e.burstLeft  = FLAME_BURST_COUNT;
                        e.burstDelay = 0;
                        e.burstDir   = e.dir;
                    }
                    e.dir *= -1;       /* cambiar dirección */
                    e.dirTimer = (e.type == ENEMY_SHOOTER) ? SHOOTER_DIR_FRAMES
                                                           : CUBE_DIR_FRAMES;
                }
            }

            /* Emitir las llamas de la ráfaga, espaciadas en el tiempo */
            if (e.type == ENEMY_SHOOTER && e.burstLeft > 0) {
                if (e.burstDelay <= 0) {
                    float fx = (e.burstDir == 1) ? (e.x + e.w) : (e.x - 1);
                    float fy = e.y + e.h / 2.0f;
                    spawnEnemyShot(fx, fy, e.burstDir);
                    e.burstLeft--;
                    e.burstDelay = FLAME_BURST_GAP;
                } else {
                    e.burstDelay--;
                }
            }

            /* Colisión con daño (hitbox actualizada tras el movimiento) */
            ex1 = e.x; ex2 = e.x + e.w;
            float kx1 = gState.worldX + 5;   /* margen interno para hitbox */
            float ky1 = gState.y + 3;
            float kx2 = gState.worldX + KIRBY_W - 5;
            float ky2 = gState.y + KIRBY_H - 3;

            bool colision = (kx1 < ex2) && (kx2 > ex1) && (ky1 < ey2) && (ky2 > ey1);

            if (colision && gState.invincibleTimer == 0) {
                gState.hp--;
                gState.invincibleTimer = INVINCIBLE_FRAMES;
                /* Knockback: empujar a Kirby en dirección opuesta */
                if (gState.worldX < e.x) {
                    gState.worldX -= 10.0f;
                } else {
                    gState.worldX += 10.0f;
                }
                gState.velY = -2.0f;
                gState.onGround = false;
                gState.jumping = true;

                /* Game over si no queda vida */
                if (gState.hp <= 0) {
                    gState.hp = 0;
                    gState.gameOver = true;
                    gState.gameOverTimer = 90; /* 90 frames ≈ 3 segundos */
                }
            }
        }

        /* Los proyectiles enemigos se mueven y colisionan en enemyShotThread */

        /* Cámara */
        float targetCam = gState.worldX - SCREEN_W / 2.0f;
        if (targetCam < 0) targetCam = 0;
        if (targetCam > WORLD_W - SCREEN_W) targetCam = WORLD_W - SCREEN_W;
        gState.cameraX += (targetCam - gState.cameraX) * 0.2f;

        pthread_mutex_unlock(&gMutex);
        usleep(33333);
    }
    return nullptr;
}

/* ── Hilo de la estrella (proyectil) ──
   Es el CONSUMIDOR del semáforo semSpit: permanece dormido en sem_wait hasta
   que el hilo de física señaliza un disparo. Entonces lanza la estrella desde
   la boca de Kirby y la anima hasta que sale de pantalla o golpea a un enemigo. */

void* starThread(void*) {
    while (true) {
        sem_wait(&semSpit);   /* dormir hasta que haya que escupir */

        /* ¿Nos despertaron solo para terminar? */
        pthread_mutex_lock(&gMutex);
        bool run = gState.running;
        if (run) {
            /* Lanzar la estrella desde la boca, en la dirección que mira Kirby */
            gStar.dir    = gState.facingDir;
            gStar.x      = (gStar.dir == 1) ? gState.worldX + KIRBY_W - 5
                                            : gState.worldX + 5;
            gStar.y      = gState.y + KIRBY_H - 4;  /* a la altura de enemigos de suelo */
            gStar.active = true;
        }
        pthread_mutex_unlock(&gMutex);
        if (!run) break;

        /* Animar la estrella en vuelo */
        bool flying = true;
        while (flying) {
            pthread_mutex_lock(&gMutex);
            if (!gState.running) { pthread_mutex_unlock(&gMutex); return nullptr; }

            gStar.x += gStar.speed * gStar.dir;

            /* ¿Golpea a algún enemigo? */
            for (int i = 0; i < NUM_ENEMIES; i++) {
                if (!enemies[i].alive) continue;
                Enemy& e = enemies[i];
                if (gStar.x >= e.x - 2 && gStar.x <= e.x + e.w + 2 &&
                    gStar.y >= e.y - 1 && gStar.y <= e.y + e.h) {
                    enemies[i].alive = false;   /* enemigo eliminado */
                    gStar.active = false;
                    flying = false;
                }
            }

            /* ¿Salió de la pantalla? */
            float screenX = gStar.x - gState.cameraX;
            if (screenX < -2 || screenX > SCREEN_W + 2) {
                gStar.active = false;
                flying = false;
            }
            pthread_mutex_unlock(&gMutex);

            if (flying) usleep(33333);
        }
    }
    return nullptr;
}

/* ── Hilo de los proyectiles enemigos ──
   Hilo dedicado que mueve y colisiona TODOS los proyectiles activos del pool.
   El hilo de física es el productor (los genera con spawnEnemyShot); este hilo
   los anima cada frame. No se bloquea en un semáforo porque debe seguir
   moviendo los proyectiles ya en vuelo; usa gMutex para el acceso compartido. */

void* enemyShotThread(void*) {
    while (true) {
        pthread_mutex_lock(&gMutex);
        if (!gState.running) { pthread_mutex_unlock(&gMutex); break; }

        /* Durante el game over no se procesan proyectiles */
        if (gState.gameOver) {
            pthread_mutex_unlock(&gMutex);
            usleep(33333);
            continue;
        }

        for (int i = 0; i < MAX_ENEMY_SHOTS; i++) {
            if (!enemyShots[i].active) continue;
            EnemyShot& s = enemyShots[i];
            s.x += s.speed * s.dir;

            /* La llama se disipa tras un tiempo corto (efecto lanzallamas) */
            s.life--;
            if (s.life <= 0) { s.active = false; continue; }

            /* Desactivar si sale de la pantalla */
            float screenX = s.x - gState.cameraX;
            if (screenX < -2 || screenX > SCREEN_W + 2) { s.active = false; continue; }

            /* ¿Golpea a Kirby? La hitbox llega hasta los pies para alcanzar las
               llamas que viajan a ras de suelo; si Kirby salta, le pasan por debajo. */
            float kx1 = gState.worldX + 5;
            float ky1 = gState.y + 3;
            float kx2 = gState.worldX + KIRBY_W - 5;
            float ky2 = gState.y + KIRBY_H;       /* antes -3: las llamas pasaban justo bajo los pies */
            if (s.x >= kx1 && s.x <= kx2 && s.y >= ky1 && s.y <= ky2) {
                s.active = false;
                if (gState.invincibleTimer == 0) {
                    gState.hp--;
                    gState.invincibleTimer = INVINCIBLE_FRAMES;
                    /* Knockback en la dirección del proyectil */
                    gState.worldX += (s.dir == 1) ? 8.0f : -8.0f;
                    gState.velY = -2.0f;
                    gState.onGround = false;
                    gState.jumping = true;
                    if (gState.hp <= 0) {
                        gState.hp = 0;
                        gState.gameOver = true;
                        gState.gameOverTimer = 90;
                    }
                }
            }
        }

        pthread_mutex_unlock(&gMutex);
        usleep(33333);
    }
    return nullptr;
}

/* ── Hilo de render ── */

void* renderThread(void*) {
    while (true) {
        /* Leer estado del juego */
        pthread_mutex_lock(&gMutex);
        if (!gState.running) { pthread_mutex_unlock(&gMutex); break; }
        float camX    = gState.cameraX;
        float kWorldX = gState.worldX;
        float ky      = gState.y;
        float velY    = gState.velY;
        bool  jumping = gState.jumping;
        bool  moving  = gState.keyLeft || gState.keyRight;
        int   facing  = gState.facingDir;
        int   hp      = gState.hp;
        int   maxHp   = gState.maxHp;
        int   invTimer = gState.invincibleTimer;
        bool  isGameOver = gState.gameOver;
        bool  isInhaling = gState.inhaling;
        /* Snapshot de la estrella */
        bool  starActive = gStar.active;
        float starX      = gStar.x;
        float starY      = gStar.y;
        int   starDir    = gStar.dir;
        pthread_mutex_unlock(&gMutex);

        /* Munición en la boca: la cuenta vive en el semáforo */
        int mouthful = 0;
        sem_getvalue(&semMouthful, &mouthful);
        bool stuffed = (mouthful > 0);

        /* Proteger TODO el dibujado con ncMutex */
        pthread_mutex_lock(&ncMutex);

        erase();

        /* Pantalla de game over */
        if (isGameOver) {
            mvprintw(SCREEN_H / 2, SCREEN_W / 2 - 5, "GAME  OVER");
            refresh();
            pthread_mutex_unlock(&ncMutex);
            usleep(1000000 / FPS);
            continue;
        }

        /* Dibujar plataformas */
        for (int i = 0; i < NUM_PLATFORMS; i++) {
            Platform& p = platforms[i];
            int sx = (int)(p.x - camX);
            for (int j = 0; j < p.width; j++) {
                int px = sx + j;
                if (px >= 0 && px < SCREEN_W && (int)p.y >= 2 && (int)p.y < SCREEN_H) {
                    wchar_t blk[] = L"═";
                    mvaddwstr((int)p.y, px, blk);
                }
            }
        }

        /* Dibujar enemigos */
        for (int i = 0; i < NUM_ENEMIES; i++) {
            if (!enemies[i].alive) continue;
            Enemy& e = enemies[i];
            const wchar_t** esprite = (e.type == ENEMY_SHOOTER) ? SPRITE_SHOOTER
                                                                : SPRITE_ENEMY;
            int ex = (int)(e.x - camX);
            for (int r = 0; r < e.h; r++) {
                int row = (int)e.y + r;
                if (row >= 2 && row < SCREEN_H && ex >= 0 && ex < SCREEN_W)
                    mvaddwstr(row, ex, esprite[r]);
            }
        }

        /* Dibujar llamas del lanzallamas (degradado según vida: sólido -> tenue) */
        for (int i = 0; i < MAX_ENEMY_SHOTS; i++) {
            if (!enemyShots[i].active) continue;
            int sx  = (int)(enemyShots[i].x - camX);
            int row = (int)enemyShots[i].y;
            if (row >= 2 && row < SCREEN_H && sx >= 0 && sx < SCREEN_W) {
                int life = enemyShots[i].life;
                const wchar_t* glyph;
                if      (life > FLAME_LIFE * 3 / 4) glyph = L"\u2588"; /* █ */
                else if (life > FLAME_LIFE / 2)     glyph = L"\u2593"; /* ▓ */
                else if (life > FLAME_LIFE / 4)     glyph = L"\u2592"; /* ▒ */
                else                                glyph = L"\u2591"; /* ░ */
                mvaddwstr(row, sx, glyph);
            }
        }

        /* Seleccionar sprite de Kirby */
        const wchar_t** sprite;
        if      (isInhaling)           sprite = SPRITE_INHALE;
        else if (stuffed && jumping && velY <  0) sprite = SPRITE_JUMP;
        else if (stuffed && jumping && velY >= 0) sprite = SPRITE_FALL;
        else if (stuffed)              sprite = SPRITE_INHALE;  /* mejillas infladas: boca llena */
        else if (jumping && velY <  0) sprite = SPRITE_JUMP;
        else if (jumping && velY >= 0) sprite = SPRITE_FALL;
        else if (moving)               sprite = SPRITE_WALK;
        else                           sprite = SPRITE_IDLE;

        /* Dibujar Kirby (voltear si mira a la izquierda) */
        /* Parpadeo durante invencibilidad */
        bool visible = (invTimer == 0) || ((invTimer / 4) % 2 == 0);
        int kx = (int)(kWorldX - camX);
        wchar_t flipped[KIRBY_W + 10];
        if (visible) {
            for (int r = 0; r < KIRBY_H; r++) {
                int row = (int)ky + r;
                if (row >= 2 && row < SCREEN_H && kx >= 0 && kx < SCREEN_W) {
                    if (facing == -1) {
                        flipLine(sprite[r], flipped, KIRBY_W + 10);
                        mvaddwstr(row, kx, flipped);
                    } else {
                        mvaddwstr(row, kx, sprite[r]);
                    }
                }
            }
        }

        /* Efecto visual de succión (aire siendo jalado hacia la boca) */
        if (isInhaling) {
            static int windFrame = 0;
            windFrame++;
            const wchar_t* puff = L"\u00b7\u2218\u00b0"; /* · ∘ ° */
            int mouthScreenX = (facing == 1) ? (kx + KIRBY_W - 5) : (kx + 5);
            int rowBase = (int)ky + 9;
            for (int d = 2; d < INHALE_RANGE; d += 3) {
                int wx    = mouthScreenX + facing * d;
                int phase = (d + windFrame) % 3;
                for (int rr = 0; rr < 3; rr++) {
                    int row = rowBase + rr;
                    if (row >= 2 && row < SCREEN_H && wx >= 0 && wx < SCREEN_W) {
                        wchar_t g[2] = { puff[(phase + rr) % 3], L'\0' };
                        mvaddwstr(row, wx, g);
                    }
                }
            }
        }

        /* Dibujar la estrella en vuelo */
        if (starActive) {
            int sx  = (int)(starX - camX);
            int row = (int)starY;
            if (row >= 2 && row < SCREEN_H && sx >= 1 && sx < SCREEN_W - 1) {
                mvaddwstr(row, sx, L"\u2605");            /* ★ */
                /* pequeña estela detrás de la estrella */
                wchar_t trail[] = L"\u2727";              /* ✧ */
                mvaddwstr(row, sx - starDir, trail);
            }
        }

        /* HUD - Vida de Kirby */
        mvprintw(0, 2, "KIRBY");
        for (int i = 0; i < maxHp; i++) {
            if (i < hp) {
                wchar_t full[] = L"■";
                mvaddwstr(0, 8 + i * 2, full);
            } else {
                wchar_t empty[] = L"□";
                mvaddwstr(0, 8 + i * 2, empty);
            }
        }

        /* HUD - Estado de la boca (E = disparar, S = tragar) */
        if (stuffed) {
            mvprintw(0, SCREEN_W - 22, "BOCA: ");
            mvaddwstr(0, SCREEN_W - 16, L"\u25cf");      /* ● enemigo en la boca */
            mvprintw(0, SCREEN_W - 14, "(E/S)");
        } else {
            mvprintw(0, SCREEN_W - 22, "BOCA: ");
            mvaddwstr(0, SCREEN_W - 16, L"\u25cb");      /* ○ vacía */
        }

        /* Estado (debug) */
        mvprintw(1, 0, "Estado: %s  HP: %d/%d",
            jumping && velY <  0 ? "SUBIENDO " :
            jumping && velY >= 0 ? "CAYENDO  " :
            moving               ? "CAMINANDO" : "IDLE     ",
            hp, maxHp);

        refresh();

        pthread_mutex_unlock(&ncMutex);

        usleep(1000000 / FPS);
    }
    return nullptr;
}

/* ── Main ── */

int main() {
    setlocale(LC_ALL, "");
    initscr(); cbreak(); noecho();
    curs_set(0); nodelay(stdscr, TRUE); keypad(stdscr, TRUE);

    /* Detectar tamaño real de la terminal */
    getmaxyx(stdscr, SCREEN_H, SCREEN_W);

    /* Inicializar mundo */
    initPlatforms();
    initEnemies();

    /* Inicializar semáforos: ambos arrancan en 0 (boca vacía, sin disparos) */
    sem_init(&semMouthful, 0, 0);
    sem_init(&semSpit,     0, 0);

    gState.worldX = 10; gState.y = platforms[0].y - KIRBY_H;
    gState.velY = 0; gState.cameraX = 0; gState.onGround = true;
    gState.jumping = false; gState.running = true;
    gState.keyLeft = false; gState.keyRight = false; gState.keyJump = false;
    gState.keyInhale = false; gState.inhaling = false;
    gState.eHeld = false;
    gState.keySwallow = false; gState.swallowHeld = false;
    gState.jumpCount = 0; gState.jumpHeld = false;
    gState.facingDir = 1;
    gState.hp = 6; gState.maxHp = 6;
    gState.invincibleTimer = 0;
    gState.gameOver = false;
    gState.gameOverTimer = 0;

    pthread_t tIn, tPh, tRe, tSt, tEs;
    pthread_create(&tIn, nullptr, inputThread,      nullptr);
    pthread_create(&tPh, nullptr, physicsThread,    nullptr);
    pthread_create(&tRe, nullptr, renderThread,     nullptr);
    pthread_create(&tSt, nullptr, starThread,       nullptr);
    pthread_create(&tEs, nullptr, enemyShotThread,  nullptr);
    pthread_join(tIn, nullptr);
    pthread_join(tPh, nullptr);
    pthread_join(tRe, nullptr);
    pthread_join(tSt, nullptr);
    pthread_join(tEs, nullptr);

    pthread_mutex_destroy(&gMutex);
    pthread_mutex_destroy(&ncMutex);
    sem_destroy(&semMouthful);
    sem_destroy(&semSpit);
    endwin();
    return 0;
}