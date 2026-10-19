#pragma once
#include <cstdint>

// Memory shared between this overlay and the C# BepInEx plugin.
// The plugin gets a pointer with Overlay_GetState() and mirrors this layout in Overlay.cs,
// so any change here must be made there too (and bump SHARED_VERSION).

#define SHARED_VERSION 6
#define MAX_ITEMS 96
#define ITEM_NAME_LEN 48
#define MAX_ESP 16
#define ESP_LABEL_LEN 24
#define DEBUG_LEN 512
#define CONSOLE_LINES 32
#define CONSOLE_LINE_LEN 128

enum EspKind : int32_t { Esp_Granny, Esp_Grandpa, Esp_Slendrina, Esp_Spider, Esp_Robot, Esp_Other };

struct EspEntry
{
    float minX, minY;    // screen box in Unity pixels, top-left origin
    float maxX, maxY;
    float distance;      // metres
    int32_t kind;        // EspKind
    int32_t onScreen;
};

struct SharedState
{
    int32_t version;
    int32_t hooked;      // overlay: 1 once it is drawing
    int32_t menuOpen;    // overlay: toggled with the menu key

    // Settings - edited by the overlay UI, applied by the plugin.
    int32_t godMode, invisible, freezeEnemies, noclip, fullbright;
    int32_t fovEnabled, timeScaleEnabled;
    int32_t espEnabled, espBoxes, espLines, espNames, espDistance;
    int32_t watermark, crosshair, notifications;
    int32_t theme;
    float fov, speedMult, noclipSpeed, timeScale;

    // One-shot actions - overlay sets, plugin runs and clears.
    int32_t actKnockGranny, actKnockGrandpa, actKnockAll;
    int32_t actSavePos, actLoadPos;
    int32_t actDayMinus, actDayPlus;
    int32_t actGiveItem;  // item index + 1
    int32_t actSaveConfig;

    // Game info - written by the plugin, shown by the overlay.
    int32_t inGame, hasSavedPos;
    float currentDay;
    float posX, posY, posZ;
    int32_t itemCount;
    char items[MAX_ITEMS][ITEM_NAME_LEN];
    int32_t espCount;
    EspEntry esp[MAX_ESP];
    char espLabel[MAX_ESP][ESP_LABEL_LEN];
    float screenW, screenH;  // Unity's Screen size, to scale ESP to the window
    char debug[DEBUG_LEN];   // diagnostics line shown in the About tab

    // v3: 3D box corners (x,y pairs; corner index bits = x,y,z of the world box)
    float espCorners[MAX_ESP][16];
    int32_t espCornersValid[MAX_ESP];
    int32_t espCorner, espFilled, esp3D, espHeadDot;
    int32_t menuParticles;

    // v3: console - last log lines, oldest first
    int32_t consoleCount;
    char console[CONSOLE_LINES][CONSOLE_LINE_LEN];

    // v4: per-entry stable id (for the enemy wheel)
    int32_t espId[MAX_ESP];
    // v4: enemy wheel - overlay sets radialOpen while the key is held, radialAction/Target on pick
    int32_t radialOpen, radialTarget, radialAction;
    // v4: shop
    int32_t coins;          // -1 = coin save not found
    int32_t inMainMenu;
    int32_t actAddCoins;    // amount to add
    int32_t actUnlockAll;

    // v5: coin save details (found by watching which PlayerPrefs the game reads)
    float coinValue;
    int32_t coinKeyType;       // 0 unknown, 1 int, 2 float
    int32_t coinKeyConfirmed;  // 1 = the game itself was seen reading this key
    char coinKey[48];
    int32_t inCapsuleShop;     // the in-house toy capsule shop is loaded
    int32_t actUnlockSkins;
    // v5: Cinematic Unity Explorer
    int32_t explorerStatus;    // 0 not installed, 1 closed, 2 open
    int32_t actToggleExplorer;

    // v6: enemies circle around the player, frozen
    int32_t orbitEnabled;
    float orbitRadius, orbitSpeed;
};

static_assert(sizeof(SharedState) == 11508, "update Overlay.cs and SHARED_VERSION");

extern SharedState g_State;
