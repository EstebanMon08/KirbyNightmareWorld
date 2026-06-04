#include <locale.h>
#include <ncurses.h>
#include <wchar.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <semaphore.h>

#define FPS         30
#define WORLD_W    1400

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



const wchar_t* SPRITE_FULL[KIRBY_H] = {
    L"                                       ",
    L"     █████                    █████    ",
    L"  ███     ██  ████████████  ██     █   ",
    L"  █        ███            ███       █  ",
    L" █                           ██     █  ",
    L" █                             █   ██  ",
    L" █                              █ ██   ",
    L"  ██                    ▒█  ▒█   ██    ",
    L"    █                   ▒█  ▒█    █    ",
    L"   █                    ██  ██    █    ",
    L"   █                ▒▒▒       ▒▒▒ █    ",
    L"   █                   █          █    ",
    L"   █                    ██████    █    ",
    L"   █                   █          █    ",
    L"    █                            █     ",
    L" █████                          ████   ",
    L" █▒▒▒██                        █▒▒▒▒█  ",
    L" █▒▒▒▒▒███                   ██▒▒▒▒▒█  ",
    L" ██▒▒▒▒▒▒▒████            ███▒▒▒▒▒██   ",
    L"   ████▒▒▒▒▒██████████████▒▒▒▒▒███     ",
    L"      ███████             █████        "
};


/* ── Iconos de power-up (retratos para el HUD) ── */
#define ICON_H 22
#define ICON_W 35

/* Icono de poder: normal (sin poder) (retrato 22x35, etiqueta incrustada) */
const wchar_t* ICON_NONE[ICON_H] = {
L" █████████████████████████████████ ",
L"█▒▒▒▒▒▒▒▒▒▒▒▒         ▒▒▒▒▒▒▒▒▒▒▒▒█",
L"█▒▒▒▒▒▒▒▒▒▒▒           ▒▒▒▒▒▒▒▒▒▒▒█",
L"█▒▒▒▒▒▒▒▒▒▒             ▒▒▒▒▒▒▒▒▒▒█",
L"█▒▒▒▒▒▒▒▒▒   █████████ ███▒▒▒▒▒▒▒▒█",
L"█▒▒▒▒▒▒▒▒  ██         █   █▒▒▒▒▒▒▒█",
L"█▒▒▒▒▒▒▒  █     █ █    █   █▒▒▒▒▒▒█",
L"█▒▒▒▒▒▒   █     █ █        █▒▒▒▒▒▒█",
L"█▒▒▒▒▒  ██    ▒▒   ▒▒      █ ▒▒▒▒▒█",
L"█▒▒▒▒  █         █         █  ▒▒▒▒█",
L"█▒▒▒   █   █             █     ▒▒▒█",
L"█▒▒    █ ████            █      ▒▒█",
L"█▒      █▒▒▒▒█         ██        ▒█",
L"█▒      █▒▒▒▒▒█      ████        ▒█",
L"█▒       █▒▒▒▒███████▒▒▒▒█       ▒█",
L"█▒▒       ████     ███████      ▒▒█",
L"█▒▒▒▒▒                       ▒▒▒▒▒█",
L"█▒▒▒▒██▒█▒▒██▒▒███▒█▒▒▒█▒▒█▒▒█▒▒▒▒█",
L"█▒▒▒▒█▒██▒█▒▒█▒█▒█▒██▒██▒█▄█▒█▒▒▒▒█",
L"█▒▒▒▒█▒▒█▒▒██▒▒█▀▄▒█▒█▒█▒█▒█▒███▒▒█",
L"█▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█",
L" █████████████████████████████████ ",
};

/* Icono de poder: rayo (retrato 22x35, etiqueta incrustada) */
const wchar_t* ICON_BEAM[ICON_H] = {
L" █████████████████████████████████ ",
L"█▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓          ▓▓▓▓▓█",
L"█▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓    ████████   ▓▓▓█",
L"█▓▓▓▓▓▓▓▓▓▓▓▓▓▓  ███        ██    █",
L"█▓▓▓▓▓▓▓▓▓▓▓▓▓  █             █████",
L"█▓▓▓▓▓▓▓▓▓▓▓▓▓ █             █    █",
L"█▓▓▓▓▓▓▓▓▓▓▓▓  █             █    █",
L"█▓▓▓▓▓▓▓▓▓▓▓  █    █   ██         █",
L"█▓▓▓▓▓▓▓▓▓   █     █  █         ███",
L"█▓▓▓▓▓▓▓▓▒  ▒█   ▒▒    ▒▒▒ █████  █",
L"█▓▓▓▓▓▓▓▓▒  ▒     █       █▒▒▒▒█  █",
L"█▓▓▓▓▓▓   ▒  ▒  ███████████▒▒▒▒▒█ █",
L"█▓▓▓▓▓▓ ▒▒▒▒  ▒▒▒▒   █▒▒▒▒█▒▒▒▒▒█ █",
L"█▓▓▓    ▒   ▒     ▓   ████ █████  █",
L"█▓▓▓ ▒▒▒▒   ▒▒▒▒ ▓▓▓             ▓█",
L"█▓   ▒   ▒▒▒      ▓          ▓    █",
L"█  ▒▒▒   ▒                  ▓▓▓   █",
L"█ ▒   ▒▒▒█▀▄ █▀▀  █  █   █   ▓    █",
L"█▒▒   ▒  █▀▄ █▀  █▄█ ██ ██        █",
L"█  ▒▒▒   █▄▀ ███ █ █ █ █ █     ▓  █",
L"█  ▒                          ▓▓▓ █",
L" █████████████████████████████████ ",
};

/* Icono de poder: espada (retrato 22x35, etiqueta incrustada) */
const wchar_t* ICON_SWORD[ICON_H] = {
L" █████████████████████████████████ ",
L"█▓▓▓▒   █   █   ▒▓▓▓▓▓▓▓▓▓▓▓▓ ▓▓▓▓█",
L"█▒▒▒   █     █   ▒▒▒▒▒▒▒▒▒▒▓▓▓▓▓▓▓█",
L"█     █       █            ▒▒▒▒▒▒▓█",
L"█▒▒▒   █  █  █   ▒▒▒▒▒▒▒▒▒▒▓▓▓▓▓▓▓█",
L"█▓▓▓▒   █ █ █   ▒▓▓▓▓ ▓▓▓▓▓▓▓▓▓▓▓▓█",
L"█▓▓▒  ▒ █ █ █ ▒  ▒▓▓▓▓▓▓▓▓ ▓▓▓▓ ▓▓█",
L"█▓▒  ▒▓▒ ▌█▐ ▒▓▒█████████▓▓▓▓▓   ▓█",
L"█▒  ▒▓▓▓█████▓██         ██▓▓▓▓ ▓▓█",
L"█  ▒▓▓▓▓▓██  █   █   █     ██▓▓▓▓▓█",
L"█ ▒▓▓▓▓▓▓█  █     █ █        █▓▓▓▓█",
L"█▒▓▓▓▓▓ ▓▓█ █  ▒▒▒   ▒▒▒      █▓▓▓█",
L"█▓▓▓▓▓▓ ▓▓▓██      █       █  █▓▓▓█",
L"█▓▓▓▓     ▓▓▓█             ███▓▓▓ █",
L"█▓▓▓▓▓▓ ▓▓▓▓▓███         ██▓▓▓▓▓  █",
L"█▓ ▓▓▓▓ ▓▓▓▓█▒▒▒█████████▒▒▒█▓▓▓▓ █",
L"█▓▓▓▓▓▓▓▓▓▓████████▓▓▓████████▓▓▓▓█",
L"█▓     █▀▀ █ █ █  ██  ███ ██     ▓█",
L"█      ▀▀█ █ █ █ █  █ █ █ █ █     █",
L"█      ███  █ █   ██  █▀▄ ██      █",
L"█▓                               ▓█",
L" █████████████████████████████████ ",
};

/* Icono de poder: fuego (retrato 22x35, etiqueta incrustada) */
const wchar_t* ICON_FIRE[ICON_H] = {
L" █████████████████████████████████ ",
L"█▓    ▒▒▒  ▓   ▒      ▓  ▓▓▓  ▒▓▓▓█",
L"█  ▒▒▒▒▒    ▒▒▒▒    ▒▒▒▒▒▒▓▓▓▓▓ ▓▓█",
L"█ ▒▒▓▓▒    ▒▒     ▒▒▒▒▒▒▒▒▒▓▒▓▓▓▓▓█",
L"█ ▓▓▓▓▒   ▒    ▒▒▒▒ ▓▓▓▓▓▓▓▓▓▓▓▓▓▓█",
L"█ ▓ ▓▓▒▒  ▒    ▒▓▓            ▓▓▓▓█",
L"█ ▓ ▓▓▓▒▒ ▒▒  ▒▒▓▓ ██████████  ▒▓▓█",
L"█ ▓ ▓▓▓▓▓▒▒▒▒  ▒▒██  █       ██▒▒▓█",
L"█▒   ▓▓ ▓▓▓ ▒▒▒  ▒  █          █▒▒█",
L"█▒    ▓▒   ▒ ▒ █▒ ▒    ███     ██▒█",
L"█▒        ▒ ▒▓ █ ▒█              ██",
L"█▓▒▒   ▒▒ ▒ ▒▓ █      ▒▒▒▒    █   █",
L"█▓▓▓▒  ▒▒▒  ▒▓  █        ▒▒   █  ██",
L"█ ▓▓ ▓▒▒▒▓▒▒▒▒▓ ███      ▄███████▓█",
L"█  ▓▓    ▓▓ ▒▒ █▒▒▒██████▒▒▒▒▒▒█▓▓█",
L"█ ▒▒ ▓▓▓▓▓▓▓▓  ███████▓▓████████▓▓█",
L"█       ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓█",
L"█▒▒▒▒    ▒ ███ █ ███ █▀▀ ▓▓▓▓▓▓▓▓▓█",
L"█▓▓▓▒▓▓▓▓▓ █▄▄ █ █ █ █▀  ▓▓▓▓▓▓▓▓▓█",
L"█▓▓▓▓▓▓▓▓▓ █   █ █▀▄ ███ ▓▓▓▓▓▓▓▓▓█",
L"█▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓█",
L" █████████████████████████████████ ",
};

/* Sprite TEMPORAL del menú principal (placeholder, reemplazar luego) */
const int MENU_H = 11;
const int MENU_W = 48;
const wchar_t* SPRITE_MENU[MENU_H] = {
L"████████████████████████████████████████████████",
L"█                                              █",
L"█     K I R B Y ' S   D R E A M   L A N D      █",
L"█                                              █",
L"█      ░░░░  [ SPRITE PLACEHOLDER ]  ░░░░      █",
L"█                                              █",
L"█         >>>   MENU  PRINCIPAL   <<<          █",
L"█                                              █",
L"█      ( reemplazar por el sprite real )       █",
L"█                                              █",
L"████████████████████████████████████████████████",
};

/* ── Enemigos ── */

#define ENEMY_H 21
#define ENEMY_W 43

/* Tipos de enemigo */
enum EnemyType { ENEMY_CUBE = 0, ENEMY_SHOOTER = 1, ENEMY_BEAM = 2, ENEMY_SWORD = 3 };

/* Frames entre cambios de dirección según el tipo */
#define CUBE_DIR_FRAMES    150  /* el cubo recorre tramos largos (~5 s) */
#define SHOOTER_DIR_FRAMES  90  /* el lanzallamas se mueve poco y voltea seguido (~3 s) */
#define BEAM_DIR_FRAMES     90  /* Waddle Doo: igual que el lanzallamas */
#define SWORD_DIR_FRAMES   150  /* espadachín: recorre la misma distancia que el cubo (Waddle Dee) */

/* Parámetros del lanzallamas (enemigo de fuego) */
#define FLAME_SPEED        2.8f /* velocidad de cada llama */
#define FLAME_LIFE         28   /* frames de vida de cada llama (mayor alcance del fuego) */
#define FLAME_BURST_COUNT  12   /* cantidad de llamas por ráfaga */
#define FLAME_BURST_GAP     2   /* frames entre una llama y la siguiente */

/* Parámetros del rayo de Waddle Doo (ataque en semicírculo) */
#define BEAM_RADIUS      20.0f  /* radio del arco frente al enemigo */
#define BEAM_LIFE          22   /* vida de cada punto del rayo (mayor: el arco se ve completo) */
#define BEAM_BURST_COUNT   9    /* puntos del rayo repartidos sobre el semicírculo */
#define BEAM_BURST_GAP      1    /* frames entre un punto y el siguiente (traza rápida) */

/* Parámetros del espadachín (ataque cuerpo a cuerpo: un slash estático al frente) */
#define SWORD_SLASH_OFFSET  6    /* distancia del slash frente al cuerpo del enemigo */
#define SWORD_SLASH_LIFE   16    /* frames que permanece visible el slash */
#define SWORD_BURST_COUNT  16    /* frames que el enemigo queda quieto mientras ataca */
#define SWORD_BURST_GAP     0    /* sin separación: el slash es un único proyectil */

const wchar_t* SPRITE_ENEMY[ENEMY_H] = {
L"                 █████████                 ",
L"             ████▓▓▓▓▓▓▓▓▓█████            ",
L"           ███▓▓▓            ▓▓██          ",
L"         ██▓▓▓▓▓               ▓▓█         ",
L"        ██▓▓▓▓▓                 ▓██        ",
L"        █▓▓▓▓▓▓      █▒   █▒    ▓▓▓█       ",
L"   █████▓▓▓▓▓        █▒   █▒      ▓█████   ",
L" ███▓▓▓▓▓▓▓          ██   ██       ▓▓▓▓███ ",
L"██▓▓▓▓▓▓▓▓▓      ▒▒▒         ▒▒▒   ▓▓▓▓▓▓██",
L"█▓▓▓▓▓▓▓▓▓▓                        ▓▓▓▓▓▓▓█",
L"█▓▓▓▓▓▓▓▓▓▓                  ███████▓▓▓▓▓▓█",
L"██▓▓▓▓▓▓▓▓▓▓              ███▒▒▒▒▒▒▒██▓▓▓██",
L"  ██▓▓▓▓▓▓▓▓▓            █▒▒▒▒▒▒▒▒▒▒▒█▓██  ",
L"    ████▓▓▓▓▓▓▓▓         █▒▒▒▒▒▒▒▒▒▒▒▒██   ",
L"        █▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓█▒▒▒▒▒▒▒▒▒▒▒▒▒█    ",
L"         ██▓▓▓▓▓▓▓▓▓▓▓▓▓█▒▒▒▒▒▒▒▒▒▒▒▒█     ",
L"           ███▓▓▓▓▓▓▓▓▓▓██▒▒▒▒▒▒▒▒▒▒█      ",
L"            █▒████████████▒▒▒▒▒▒▒███       ",
L"            █▒▒▒▒▒▒▒▒▒▒▒█ ███████          ",
L"             ███▒▒▒▒▒▒▒█                   ",
L"               ████████                    ",
};

/* Sprite del lanzador: bloque más sólido (▓) con dos ojos, para distinguirlo */
const wchar_t* SPRITE_SHOOTER[ENEMY_H] = {
L"      ▒                                    ",
L"         ▓ ▓▓                              ",
L"          ▓ ▓▓▓▓                           ",
L"   ▒  ▓ ▓ ▓    ▒▓▓ ██████    ████          ",
L"      ▓▒▒▓▒▒    ███▒▒▒▒▒▒████▒▒▒▒█         ",
L"     ▓ ▓  ▓ ▓▓  ▒▒▒▒▒▒███▒▒▒▒▒███▒█        ",
L"    ▓▒▒▒▓      ▒▒▒▒▒▒███ █▒▒▒███ █▒█       ",
L"    ▓▒▓        ▒▒▒▒▒█ ███ ▒▒█ ███ ▒█       ",
L"     ▓▒    ▓▓ ▒▒▒▒▒▒█ ████▒▒█ ████▒█       ",
L"   ▓▓▒▓     ▓▓▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒██     █      ",
L"   ▒▓▓      ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█   ███  █     ",
L"   ▓▓   ▓▓ ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█  █████ █     ",
L"   ▒▓▓     ▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒▒█   ███  █     ",
L"    ▒▓▒▓   ▓▒▒▒▒▒▒▒▒▒▒▒█████▒██     █      ",
L"   ▓▓▒ ▓  ▓▓▓▓▒▒▒▒▒▒▒██     █▒▒█████       ",
L"    ▓▓     ▓▓▓▓▓▒▒▒▒█        █▒▒▒█         ",
L"     ▓▓▓▓▓▒███▓▓▓▓▓▓█        █▒██          ",
L"          ▓▓▒█████▓▓██      ███            ",
L"              █       ██████               ",
L"              ██     ██                    ",
L"                █████                      "
};

/* Sprite de Waddle Doo: cuerpo redondo con un solo ojo grande */
const wchar_t* SPRITE_BEAM[ENEMY_H] = {
L"            ██   ██                       ",
L"              █  █                        ",
L"               ██████████████             ",
L"            ████▓▓▓▓▓▓▓▓▓▓▓▓▓███          ",
L"          ██▓▓▓▓▓▓▓          ▓▓▓██        ",
L"         █▓▓▓▓▓▓▓     ███████   ▓▓█       ",
L"        █▓▓▓▓▓▓     ██████  ███  ▓▓█      ",
L"        █▓▓▓▓▓    ███████    ████ ▓█      ",
L"    ████▓▓▓▓▓▓    ████████  █████ ▓▓███   ",
L"  ██▓▓▓▓▓▓▓▓▓▓    ███████████████ ▓▓▓▓▓██ ",
L" █▓▓▓▓▓▓▓▓▓▓▓▓    █████████ █████ ▓▓▓▓▓▓▓█",
L" █▓▓▓▓▓▓▓▓▓▓▓▓      ████████████  ▓▓▓▓▓▓▓█",
L" █▓▓▓▓▓▓▓▓▓▓▓▓▓      █████████   ▓▓▓▓▓▓▓▓█",
L"  █▓▓▓▓▓▓▓▓▓▓▓▓▓          ████████▓▓▓▓▓▓█ ",
L"   ███▓▓▓▓▓▓▓▓▓▓▓▓▓      ██       ██▓▓██  ",
L"      ████▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓█         ███    ",
L"          ██▓▓▓▓▓▓▓▓▓▓▓▓█          █      ",
L"           ███▓▓▓▓▓▓▓▓▓▓█         ██      ",
L"           █  ████████████      ███       ",
L"           ██          █ ████████         ",
L"             ██████████                   "
};

/* Sprite del espadachín: cuerpo redondo con una espada alzada a la derecha.
   La hoja usa '/' para que al voltear (mirrorChar) apunte hacia el otro lado. */
const wchar_t* SPRITE_SWORD[ENEMY_H] = {
L"                              //          ",
L"                             //           ",
L"                            //            ",
L"          ████████         //             ",
L"        ██▓▓▓▓▓▓▓▓██       //              ",
L"       █▓▓▓▓▓▓▓▓▓▓▓▓█  ███████             ",
L"      █▓▓▓▓▓▓▓▓▓▓▓▓▓▓█  //                 ",
L"     █▓▓▓▒▒▓▓▓▓▓▓▒▒▓▓▓█ //                 ",
L"     █▓▓▓▒▒▓▓▓▓▓▓▒▒▓▓▓█//                  ",
L"    █▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓█                   ",
L"    █▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓█                   ",
L"    █▓▓▓▓▓▓████████▓▓▓▓█                   ",
L"    █▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓█                   ",
L"     █▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓█                    ",
L"     █▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓█                    ",
L"      █▓▓▓▓▓▓▓▓▓▓▓▓▓▓█                     ",
L"       █▓▓▓▓▓▓▓▓▓▓▓▓█                      ",
L"        ██▓▓▓▓▓▓▓▓██                       ",
L"          ████████                        ",
L"          ██    ██                        ",
L"         ██      ██                       "
};

/* Sprite de proyectil (bola): usado por TODOS los proyectiles del juego */
#define PROJ_H 9
#define PROJ_W 17
const wchar_t* SPRITE_PROJECTILE[PROJ_H] = {
L"      ██████     ",
L"   ███▒▒▒▒▒▒███  ",
L"  █▒▒▒      ▒▒▒█ ",
L" █▒▒          ▒▒█",
L" █▒            ▒█",
L" █▒▒          ▒▒█",
L"  █▒▒▒      ▒▒▒█ ",
L"   ███▒▒▒▒▒▒███  ",
L"      ██████     "
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

const int NUM_ENEMIES = 5;
Enemy enemies[NUM_ENEMIES];

/* ── Proyectiles de los enemigos (llamas del lanzallamas / rayo de Waddle Doo) ── */

struct EnemyShot {
    float x, y;
    float vx, vy;      /* velocidad por componente (llama: recto; rayo: estático en el arco) */
    int   dir;         /* dirección para el knockback al golpear a Kirby */
    bool  active;
    int   life;        /* frames de vida restantes; al llegar a 0 se disipa */
    int   life0;       /* vida inicial (para el degradado visual) */
};
const int MAX_ENEMY_SHOTS = 32;  /* pool amplio: una ráfaga mantiene varias partículas vivas */
EnemyShot enemyShots[MAX_ENEMY_SHOTS];

/* Activa el primer proyectil libre del pool (llamar con gMutex tomado) */
void spawnEnemyShot(float x, float y, int dir, float vx, float vy, int life) {
    for (int i = 0; i < MAX_ENEMY_SHOTS; i++) {
        if (!enemyShots[i].active) {
            enemyShots[i] = { x, y, vx, vy, dir, true, life, life };
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
    enemies[3] = { 560, gy, ENEMY_W, ENEMY_H, true, -1, 0.4f, BEAM_DIR_FRAMES,    ENEMY_BEAM,    0, 0, 0 };
    enemies[4] = { 700, gy, ENEMY_W, ENEMY_H, true,  1, 1.0f, SWORD_DIR_FRAMES,   ENEMY_SWORD,   0, 0, 0 };

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

/* ── Llamas del poder de fuego de Kirby (lastiman enemigos, no a Kirby) ── */

struct KirbyShot {
    float x, y, vx, vy;
    bool  active;
    int   life;
};
const int MAX_KIRBY_SHOTS = 32;
KirbyShot kirbyShots[MAX_KIRBY_SHOTS];

/* Activa el primer proyectil libre del pool de Kirby (llamar con gMutex tomado).
   Sirve para ambos poderes: la llama recta (vx!=0) y el punto de rayo (vx=vy=0). */
void spawnKirbyShot(float x, float y, float vx, float vy, int life) {
    for (int i = 0; i < MAX_KIRBY_SHOTS; i++) {
        if (!kirbyShots[i].active) {
            kirbyShots[i] = { x, y, vx, vy, true, life };
            return;
        }
    }
}

/* ── Plataformas ── */

struct Platform { float x, y; int width; };
const int NUM_PLATFORMS = 15;
Platform platforms[NUM_PLATFORMS];

void initPlatforms() {
    int groundY = SCREEN_H - 3;
    /* Suelo principal (varios tramos a lo largo de todo el mundo) */
    platforms[0]  = {   0, (float)groundY, 180};   /* 0   - 180  */
    platforms[1]  = { 200, (float)groundY, 120};   /* 200 - 320  */
    platforms[2]  = { 350, (float)groundY, 250};   /* 350 - 600  */
    platforms[3]  = { 620, (float)groundY, 300};   /* 620 - 920  (bajo el espadachín @700) */
    platforms[4]  = { 950, (float)groundY, 250};   /* 950 - 1200 */
    platforms[5]  = {1230, (float)groundY, 170};   /* 1230- 1400 */
    /* Plataformas flotantes (decorado y plataformeo) */
    platforms[6]  = {  30, (float)(groundY - 8),  25};
    platforms[7]  = {  90, (float)(groundY - 15), 20};
    platforms[8]  = { 160, (float)(groundY - 20), 18};
    platforms[9]  = { 260, (float)(groundY - 16), 22};
    platforms[10] = { 380, (float)(groundY - 10), 30};
    platforms[11] = { 680, (float)(groundY - 14), 28};
    platforms[12] = { 820, (float)(groundY - 20), 26};
    platforms[13] = {1010, (float)(groundY - 12), 30};
    platforms[14] = {1120, (float)(groundY - 18), 24};
}

/* ── Estado del juego ── */

#define INVINCIBLE_FRAMES 60  /* frames de invencibilidad tras recibir daño */

/* ── Succión ── */
#define INHALE_RANGE 60   /* alcance de la zona de succión frente a Kirby (columnas) */
#define INHALE_PULL  5.0f /* velocidad con que el enemigo es arrastrado a la boca */
#define MOUTHFUL_MAX 1    /* Kirby solo puede tener un enemigo en la boca a la vez */

/* Modos del juego, coordinados por la variable de condición gModeCond:
   los hilos de simulación (physics, enemyShot) esperan dormidos mientras
   el modo no sea JUGANDO; input los despierta al pasar a JUGANDO. */
enum GameMode { MODE_MENU = 0, MODE_PLAYING = 1, MODE_GAMEOVER = 2 };

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
    int   mode;            /* MODE_MENU / MODE_PLAYING / MODE_GAMEOVER */
    int   powerup;         /* poder actual (POWER_NONE / POWER_FIRE) */
    int   mouthEnemyType;  /* tipo del enemigo que tiene en la boca (-1 si ninguno) */
    int   kFireBurstLeft;  /* partículas que faltan en la ráfaga de ataque de Kirby */
    int   kFireBurstDelay; /* frames hasta la siguiente partícula */
    int   kFireBurstDir;   /* dirección de la ráfaga de ataque de Kirby */
    int   kBurstKind;      /* tipo de la ráfaga en curso (POWER_FIRE / POWER_BEAM) */
};

/* Poderes de Kirby */
enum PowerType { POWER_NONE = 0, POWER_FIRE = 1, POWER_BEAM = 2, POWER_SWORD = 3 };

/* Mutex para estado del juego */
pthread_mutex_t gMutex = PTHREAD_MUTEX_INITIALIZER;
/* Variable de condición: coordina los cambios de modo (MENU/JUGANDO/GAME OVER).
   Va emparejada con gMutex. Los hilos de simulación esperan en ella mientras el
   modo no sea JUGANDO; el hilo de input hace broadcast al cambiar a JUGANDO. */
pthread_cond_t gModeCond = PTHREAD_COND_INITIALIZER;
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
    /* Vaciar la boca: dejar el contador de munición en 0 */
    while (sem_trywait(&semMouthful) == 0) { /* drenar */ }
    gStar.active = false;
    /* Reiniciar poder y llamas de Kirby */
    gState.powerup = POWER_NONE;
    gState.mouthEnemyType = -1;
    gState.kFireBurstLeft = 0;
    gState.kFireBurstDelay = 0;
    gState.kFireBurstDir = 1;
    gState.kBurstKind = POWER_NONE;
    for (int i = 0; i < MAX_KIRBY_SHOTS; i++) kirbyShots[i].active = false;
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
        bool sawSwallow = false, sawConfirm = false;

        pthread_mutex_lock(&ncMutex);
        while ((ch = getch()) != ERR) {
            switch (ch) {
                case 'a': case 'A': sawLeft  = true; break;
                case 'd': case 'D': sawRight = true; break;
                case 'w': case 'W': case ' ': sawJump = true; break;
                case 'e': case 'E': sawInhale = true; break;
                case 's': case 'S': sawSwallow = true; break;
                case '\n': case '\r': case KEY_ENTER: sawConfirm = true; break;
                case 'q': case 'Q': case 27:
                    pthread_mutex_unlock(&ncMutex);
                    pthread_mutex_lock(&gMutex);
                    gState.running = false;
                    /* Despertar a los hilos que pudieran estar dormidos en la
                       variable de condición (menú / game over) para que terminen */
                    pthread_cond_broadcast(&gModeCond);
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
        if (gState.mode == MODE_PLAYING) {
            gState.keyLeft   = (noLeftCount   < RELEASE_THRESHOLD);
            gState.keyRight  = (noRightCount  < RELEASE_THRESHOLD);
            gState.keyJump   = (noJumpCount   < RELEASE_THRESHOLD);
            gState.keyInhale = (noInhaleCount < RELEASE_THRESHOLD);
            gState.keySwallow = (noSwallowCount < RELEASE_THRESHOLD);
        } else {
            /* En MENU o GAME OVER, Enter arranca/reinicia la partida. Aquí el
               input SEÑALA la variable de condición para despertar a physics y
               enemyShot, que están dormidos esperando el modo JUGANDO. */
            if (sawConfirm) {
                resetGame();
                gState.mode = MODE_PLAYING;
                pthread_cond_broadcast(&gModeCond);
            }
        }
        pthread_mutex_unlock(&gMutex);

        usleep(8000);
    }
    return nullptr;
}

/* ── Hilo de fisica ── */

void* physicsThread(void*) {
    while (true) {
        pthread_mutex_lock(&gMutex);
        /* Dormir mientras no estemos jugando (menú o game over). Se despierta
           con el broadcast del input. El while protege de despertares espurios. */
        while (gState.running && gState.mode != MODE_PLAYING)
            pthread_cond_wait(&gModeCond, &gMutex);
        if (!gState.running) { pthread_mutex_unlock(&gMutex); break; }

        /* Estado actual de la boca (la cuenta vive en el semáforo) */
        int curMouth;
        sem_getvalue(&semMouthful, &curMouth);
        bool mouthFull = (curMouth > 0);
        bool hasPower  = (gState.powerup != POWER_NONE);

        /* Tecla E (flanco):
           - con poder (fuego o rayo) -> lanza la ráfaga del ataque correspondiente
           - boca llena               -> escupe al enemigo como estrella
           - boca vacía               -> (inhala, manejado abajo por nivel) */
        if (gState.keyInhale && !gState.eHeld) {
            if (hasPower) {
                gState.kFireBurstLeft  = (gState.powerup == POWER_FIRE)  ? FLAME_BURST_COUNT :
                                         (gState.powerup == POWER_BEAM)  ? BEAM_BURST_COUNT  :
                                                                           SWORD_BURST_COUNT;
                gState.kFireBurstDelay = 0;
                gState.kFireBurstDir   = gState.facingDir;
                gState.kBurstKind      = gState.powerup;   /* fijar el tipo de la ráfaga */
            } else if (mouthFull) {
                if (sem_trywait(&semMouthful) == 0) {
                    sem_post(&semSpit);            /* señalizar al hilo de la estrella */
                }
            }
        }
        gState.eHeld = gState.keyInhale;

        /* Succión: solo sin poder, con la boca vacía y en el suelo */
        gState.inhaling = gState.keyInhale && gState.onGround && !mouthFull && !hasPower;

        /* Disparando con un poder: mientras mantiene E (o mientras la ráfaga
           sigue saliendo) Kirby queda anclado, igual que al inhalar. */
        bool firing = hasPower && (gState.keyInhale || gState.kFireBurstLeft > 0);

        /* Tecla S (flanco) - tragar / soltar poder:
           - boca llena  -> digiere al enemigo; si era de fuego, gana el poder de fuego
           - con poder    -> suelta el poder (vuelve a poder succionar) */
        if (gState.keySwallow && !gState.swallowHeld) {
            if (mouthFull) {
                if (sem_trywait(&semMouthful) == 0) {
                    if (gState.mouthEnemyType == ENEMY_SHOOTER)
                        gState.powerup = POWER_FIRE;   /* lanzallamas -> poder de fuego */
                    else if (gState.mouthEnemyType == ENEMY_BEAM)
                        gState.powerup = POWER_BEAM;   /* Waddle Doo  -> poder de rayo */
                    else if (gState.mouthEnemyType == ENEMY_SWORD)
                        gState.powerup = POWER_SWORD;  /* espadachín   -> poder de espada */
                    else
                        gState.powerup = POWER_NONE;   /* enemigo común -> sin poder */
                    gState.mouthEnemyType = -1;
                }
            } else if (hasPower) {
                gState.powerup = POWER_NONE;           /* soltar el poder actual */
            }
            gState.swallowHeld = true;
        }
        if (!gState.keySwallow) gState.swallowHeld = false;

        /* Emitir la ráfaga del ataque de Kirby (fuego recto, rayo en arco o slash) */
        if (gState.kFireBurstLeft > 0) {
            if (gState.kFireBurstDelay <= 0) {
                if (gState.kBurstKind == POWER_FIRE) {
                    /* Fuego: llama recta desde la boca, a media altura */
                    float fx = (gState.kFireBurstDir == 1) ? (gState.worldX + KIRBY_W - 5)
                                                           : (gState.worldX + 5);
                    float fy = gState.y + KIRBY_H / 2;
                    spawnKirbyShot(fx, fy, FLAME_SPEED * gState.kFireBurstDir, 0.0f, FLAME_LIFE);
                    gState.kFireBurstDelay = FLAME_BURST_GAP;
                } else if (gState.kBurstKind == POWER_BEAM) {
                    /* Rayo: cada punto se coloca sobre un semicírculo frente a Kirby.
                       El índice de la ráfaga define el ángulo (+90° -> -90°). */
                    int idx = BEAM_BURST_COUNT - gState.kFireBurstLeft;  /* 0..COUNT-1 */
                    float t = (BEAM_BURST_COUNT > 1)
                                  ? (float)idx / (BEAM_BURST_COUNT - 1) : 0.0f;
                    float theta = (float)(M_PI / 2.0) - (float)M_PI * t;
                    float originX = (gState.kFireBurstDir == 1) ? (gState.worldX + KIRBY_W - 5)
                                                                : (gState.worldX + 5);
                    float originY = gState.y + KIRBY_H / 2.0f;
                    float px = originX + BEAM_RADIUS * cosf(theta) * gState.kFireBurstDir;
                    float py = originY - BEAM_RADIUS * sinf(theta);
                    spawnKirbyShot(px, py, 0.0f, 0.0f, BEAM_LIFE);
                    gState.kFireBurstDelay = BEAM_BURST_GAP;
                } else {
                    /* Espada: un único slash estático frente a Kirby, igual que el del
                       espadachín. Se crea solo en el primer frame del ataque; el resto
                       del burst únicamente mantiene a Kirby anclado mientras dura. */
                    if (gState.kFireBurstLeft == SWORD_BURST_COUNT) {
                        float sx = (gState.kFireBurstDir == 1)
                                       ? (gState.worldX + KIRBY_W + SWORD_SLASH_OFFSET)
                                       : (gState.worldX - SWORD_SLASH_OFFSET);
                        float sy = gState.y + KIRBY_H / 2.0f;
                        spawnKirbyShot(sx, sy, 0.0f, 0.0f, SWORD_SLASH_LIFE);
                    }
                    gState.kFireBurstDelay = SWORD_BURST_GAP;
                }
                gState.kFireBurstLeft--;
            } else {
                gState.kFireBurstDelay--;
            }
        }

        /* Mover las llamas de Kirby y colisionar con enemigos */
        for (int i = 0; i < MAX_KIRBY_SHOTS; i++) {
            if (!kirbyShots[i].active) continue;
            KirbyShot& ks = kirbyShots[i];
            ks.x += ks.vx;
            ks.y += ks.vy;
            ks.life--;
            if (ks.life <= 0) { ks.active = false; continue; }
            float screenX = ks.x - gState.cameraX;
            if (screenX < -2 || screenX > SCREEN_W + 2) { ks.active = false; continue; }
            for (int j = 0; j < NUM_ENEMIES; j++) {
                if (!enemies[j].alive) continue;
                Enemy& en = enemies[j];
                if (ks.x >= en.x - 2 && ks.x <= en.x + en.w + 2 &&
                    ks.y >= en.y - 1 && ks.y <= en.y + en.h) {
                    en.alive = false;       /* la llama elimina al enemigo */
                    ks.active = false;
                    break;
                }
            }
        }

        /* Movimiento (bloqueado si está succionando o disparando un poder) */
        if (!gState.inhaling && !firing) {
            if (gState.keyLeft)  { gState.worldX -= 2.0f; gState.facingDir = -1; }
            if (gState.keyRight) { gState.worldX += 2.0f; gState.facingDir =  1; }
        }
        if (gState.worldX < 0) gState.worldX = 0;
        if (gState.worldX > WORLD_W - KIRBY_W) gState.worldX = WORLD_W - KIRBY_W;

        /* Salto (bloqueado si está succionando o disparando un poder) */
        if (gState.keyJump && !gState.jumpHeld && !gState.inhaling && !firing) {
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
                        gState.mouthEnemyType = e.type;  /* recordar qué tragó */
                        sem_post(&semMouthful);  /* +1 munición disponible */
                    }
                    /* Si la boca está llena, el enemigo simplemente no se traga */
                }
                /* Mientras es succionado no se mueve solo ni hace daño */
                continue;
            }

            /* Movimiento normal del enemigo. Los enemigos que atacan
               (lanzallamas y Waddle Doo) se detienen mientras disparan. */
            bool isAttacker = (e.type == ENEMY_SHOOTER || e.type == ENEMY_BEAM ||
                               e.type == ENEMY_SWORD);
            bool bursting = (isAttacker && e.burstLeft > 0);
            if (!bursting) {
                e.x += e.speed * e.dir;
                e.dirTimer--;
                if (e.dirTimer <= 0) {
                    /* Al cambiar de dirección, los atacantes inician su ráfaga
                       hacia donde miran en ese momento. */
                    if (e.type == ENEMY_SHOOTER) {
                        e.burstLeft = FLAME_BURST_COUNT;
                        e.burstDelay = 0;
                        e.burstDir = e.dir;
                    } else if (e.type == ENEMY_BEAM) {
                        e.burstLeft = BEAM_BURST_COUNT;
                        e.burstDelay = 0;
                        e.burstDir = e.dir;
                    } else if (e.type == ENEMY_SWORD) {
                        e.burstLeft = SWORD_BURST_COUNT;
                        e.burstDelay = 0;
                        e.burstDir = e.dir;
                    }
                    e.dir *= -1;       /* cambiar dirección */
                    if      (e.type == ENEMY_SHOOTER) e.dirTimer = SHOOTER_DIR_FRAMES;
                    else if (e.type == ENEMY_BEAM)    e.dirTimer = BEAM_DIR_FRAMES;
                    else if (e.type == ENEMY_SWORD)   e.dirTimer = SWORD_DIR_FRAMES;
                    else                              e.dirTimer = CUBE_DIR_FRAMES;
                }
            }

            /* Emitir las partículas de la ráfaga, espaciadas en el tiempo */
            if (isAttacker && e.burstLeft > 0) {
                if (e.burstDelay <= 0) {
                    if (e.type == ENEMY_SHOOTER) {
                        /* Llama: sale recta desde la boca, a media altura */
                        float fx = (e.burstDir == 1) ? (e.x + e.w) : (e.x - 1);
                        float fy = e.y + e.h / 2.0f;
                        spawnEnemyShot(fx, fy, e.burstDir,
                                       FLAME_SPEED * e.burstDir, 0.0f, FLAME_LIFE);
                        e.burstDelay = FLAME_BURST_GAP;
                    } else if (e.type == ENEMY_BEAM) {
                        /* Rayo: cada punto se coloca sobre un semicírculo frente
                           al enemigo. El índice de la ráfaga define el ángulo,
                           barriendo de arriba (+90°) hacia abajo (-90°). */
                        int idx = BEAM_BURST_COUNT - e.burstLeft;  /* 0..COUNT-1 */
                        float t = (BEAM_BURST_COUNT > 1)
                                      ? (float)idx / (BEAM_BURST_COUNT - 1) : 0.0f;
                        float theta = (float)(M_PI / 2.0) - (float)M_PI * t;
                        float originX = (e.burstDir == 1) ? (e.x + e.w) : (e.x - 1);
                        float originY = e.y + e.h / 2.0f;
                        float px = originX + BEAM_RADIUS * cosf(theta) * e.burstDir;
                        float py = originY - BEAM_RADIUS * sinf(theta);
                        spawnEnemyShot(px, py, e.burstDir, 0.0f, 0.0f, BEAM_LIFE);
                        e.burstDelay = BEAM_BURST_GAP;
                    } else {
                        /* Espada: un único slash estático frente al enemigo. Se crea
                           solo en el primer frame del ataque; el resto del burst
                           únicamente lo mantiene quieto mientras el slash es visible. */
                        if (e.burstLeft == SWORD_BURST_COUNT) {
                            float sx = (e.burstDir == 1) ? (e.x + e.w + SWORD_SLASH_OFFSET)
                                                         : (e.x - SWORD_SLASH_OFFSET);
                            float sy = e.y + e.h / 2.0f;
                            spawnEnemyShot(sx, sy, e.burstDir, 0.0f, 0.0f, SWORD_SLASH_LIFE);
                        }
                        e.burstDelay = SWORD_BURST_GAP;
                    }
                    e.burstLeft--;
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
                    gState.mode = MODE_GAMEOVER;   /* el próximo ciclo dormirá en la cond var */
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
            gStar.y      = gState.y + KIRBY_H / 2;  /* a la altura de la boca, no de los pies */
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
        /* Dormir mientras no estemos jugando (menú o game over) */
        while (gState.running && gState.mode != MODE_PLAYING)
            pthread_cond_wait(&gModeCond, &gMutex);
        if (!gState.running) { pthread_mutex_unlock(&gMutex); break; }

        for (int i = 0; i < MAX_ENEMY_SHOTS; i++) {
            if (!enemyShots[i].active) continue;
            EnemyShot& s = enemyShots[i];
            s.x += s.vx;
            s.y += s.vy;

            /* La partícula se disipa tras un tiempo corto */
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
                        gState.mode = MODE_GAMEOVER;
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

/* Dibuja el sprite de bola centrado en (cxScreen, cyScreen).
   Salta los espacios para que el centro hueco y los bordes sean transparentes.
   Debe llamarse con ncMutex tomado (como el resto del dibujado). */
void drawProjectile(int cxScreen, int cyScreen) {
    for (int r = 0; r < PROJ_H; r++) {
        int row = cyScreen - PROJ_H / 2 + r;
        if (row < 2 || row >= SCREEN_H) continue;
        const wchar_t* line = SPRITE_PROJECTILE[r];
        for (int c = 0; line[c] != L'\0'; c++) {
            if (line[c] == L' ') continue;
            int col = cxScreen - PROJ_W / 2 + c;
            if (col < 0 || col >= SCREEN_W) continue;
            wchar_t ch[2] = { line[c], L'\0' };
            mvaddwstr(row, col, ch);
        }
    }
}

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
        bool  isMenu     = (gState.mode == MODE_MENU);
        bool  isGameOver = (gState.mode == MODE_GAMEOVER);
        bool  isInhaling = gState.inhaling;
        int   powerup    = gState.powerup;
        /* Snapshot de la estrella */
        bool  starActive = gStar.active;
        float starX      = gStar.x;
        float starY      = gStar.y;
        pthread_mutex_unlock(&gMutex);

        /* Munición en la boca: la cuenta vive en el semáforo */
        int mouthful = 0;
        sem_getvalue(&semMouthful, &mouthful);
        bool stuffed = (mouthful > 0);

        /* Proteger TODO el dibujado con ncMutex */
        pthread_mutex_lock(&ncMutex);

        erase();

        if (isMenu) {
            /* Pantalla de menú: dibuja el sprite placeholder centrado y el prompt */
            int top = SCREEN_H / 2 - MENU_H / 2;
            int left = SCREEN_W / 2 - MENU_W / 2;
            for (int i = 0; i < MENU_H; i++) {
                int yy = top + i;
                if (yy >= 0 && yy < SCREEN_H && left >= 0)
                    mvaddwstr(yy, left, SPRITE_MENU[i]);
            }
            mvprintw(top + MENU_H + 1, SCREEN_W / 2 - 13, "PRESIONA ENTER PARA JUGAR");
            mvprintw(top + MENU_H + 2, SCREEN_W / 2 -  9, "Q / ESC para salir");

            refresh();
            pthread_mutex_unlock(&ncMutex);
            usleep(1000000 / FPS);
            continue;
        }

        if (isGameOver) {
            const char* gameOver[] = {
                " █████    ██    ███    ███ ███████       █████  ███  ███ ███████ ██████   ",
                "██    █   █ ██    ███  ███   ██   █      ██   ██  ██   ██  ██   █  ██  ██ ",
                "██        █ ██    ██ █ █ █   ████        ██   ██   █  ██   ████    ██  ██ ",
                "██   ███ ██████   ██ ██  █   ██          ██   ██   ██ ██   ██      ████   ",
                "██    █  █   ██   ██     █   ██   █      ██   ██    ███    ██   █  ██  ██ ",
                "  █████ ███  ███ ███     ██ ███████       █████     ██    ███████ ███  ███"
            };

            int numLines = 6;
            int startY = SCREEN_H / 2 - numLines / 2;
            int startX = SCREEN_W / 2 - 37; // ~mitad del ancho del sprite

            for (int i = 0; i < numLines; i++) {
                mvprintw(startY + i, startX, "%s", gameOver[i]);
            }
            mvprintw(startY + numLines + 2, SCREEN_W / 2 - 16,
                     "PRESIONA ENTER PARA REINICIAR");

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

        /* Dibujar enemigos (voltear el sprite según su dirección) */
        wchar_t eflipped[ENEMY_W + 10];
        for (int i = 0; i < NUM_ENEMIES; i++) {
            if (!enemies[i].alive) continue;
            Enemy& e = enemies[i];
            const wchar_t** esprite;
            if      (e.type == ENEMY_SHOOTER) esprite = SPRITE_SHOOTER;
            else if (e.type == ENEMY_BEAM)    esprite = SPRITE_BEAM;
            else if (e.type == ENEMY_SWORD)   esprite = SPRITE_SWORD;
            else                              esprite = SPRITE_ENEMY;
            int ex = (int)(e.x - camX);
            /* Mientras ataca, el sprite mira hacia el ataque (burstDir); solo se
               voltea a la nueva dirección cuando la ráfaga termina. */
            int edir = e.dir;
            if ((e.type == ENEMY_SHOOTER || e.type == ENEMY_BEAM ||
                 e.type == ENEMY_SWORD) && e.burstLeft > 0)
                edir = e.burstDir;
            for (int r = 0; r < e.h; r++) {
                int row = (int)e.y + r;
                if (row >= 2 && row < SCREEN_H && ex >= 0 && ex < SCREEN_W) {
                    if (edir == -1) {
                        flipLine(esprite[r], eflipped, ENEMY_W + 10);
                        mvaddwstr(row, ex, eflipped);
                    } else {
                        mvaddwstr(row, ex, esprite[r]);
                    }
                }
            }
        }

        /* Dibujar partículas enemigas (llamas y rayo) con el sprite de bola */
        for (int i = 0; i < MAX_ENEMY_SHOTS; i++) {
            if (!enemyShots[i].active) continue;
            int sx  = (int)(enemyShots[i].x - camX);
            int row = (int)enemyShots[i].y;
            drawProjectile(sx, row);
        }

        /* Seleccionar sprite de Kirby */
        const wchar_t** sprite;
        if      (stuffed)              sprite = SPRITE_FULL;   /* enemigo en la boca */
        else if (isInhaling)           sprite = SPRITE_INHALE;
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
                if (sprite[r] == nullptr) continue;   /* fila ausente: no dibujar */
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

        /* Dibujar el proyectil que escupe Kirby con el sprite de bola */
        if (starActive) {
            int sx  = (int)(starX - camX);
            int row = (int)starY;
            drawProjectile(sx, row);
        }

        /* Dibujar las llamas del poder de fuego de Kirby */
        for (int i = 0; i < MAX_KIRBY_SHOTS; i++) {
            if (!kirbyShots[i].active) continue;
            int sx  = (int)(kirbyShots[i].x - camX);
            int row = (int)kirbyShots[i].y;
            drawProjectile(sx, row);
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

        /* HUD - Estado de la boca (E = disparar, S = tragar). Va a la izquierda,
           debajo de la vida, porque el retrato del poder ocupa la esquina derecha. */
        if (stuffed) {
            mvprintw(2, 2, "BOCA: ");
            mvaddwstr(2, 8, L"\u25cf");      /* ● enemigo en la boca */
            mvprintw(2, 10, "(E/S)");
        } else {
            mvprintw(2, 2, "BOCA: ");
            mvaddwstr(2, 8, L"\u25cb");      /* ○ vacía */
        }

        /* HUD - Retrato del poder actual de Kirby (esquina superior derecha).
           Se dibuja como badge opaco (incluyendo espacios) para que se vea como
           un retrato enmarcado y no se transparente el fondo del juego. */
        const wchar_t** picon = (powerup == POWER_FIRE)  ? ICON_FIRE  :
                                (powerup == POWER_BEAM)  ? ICON_BEAM  :
                                (powerup == POWER_SWORD) ? ICON_SWORD : ICON_NONE;
        int iconLeft = SCREEN_W - ICON_W - 1;
        for (int r = 0; r < ICON_H; r++) {
            if (r >= SCREEN_H) break;
            for (int c = 0; c < ICON_W; c++) {
                int xx = iconLeft + c;
                if (xx < 1 || xx >= SCREEN_W) continue;
                wchar_t cell[2] = { picon[r][c], L'\0' };
                mvaddwstr(r, xx, cell);
            }
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
    gState.mode = MODE_MENU;   /* el juego arranca en el menú principal */
    gState.powerup = POWER_NONE;
    gState.mouthEnemyType = -1;
    gState.kFireBurstLeft = 0;
    gState.kFireBurstDelay = 0;
    gState.kFireBurstDir = 1;
    gState.kBurstKind = POWER_NONE;
    for (int i = 0; i < MAX_KIRBY_SHOTS; i++) kirbyShots[i].active = false;

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
    pthread_cond_destroy(&gModeCond);
    sem_destroy(&semMouthful);
    sem_destroy(&semSpit);
    endwin();
    return 0;
}