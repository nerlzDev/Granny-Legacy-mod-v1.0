// Standalone build: the cheats in C++, talking to the game through IL2CPP directly.
// This replaces the C# BepInEx plugin, so one injected DLL is all that's needed.
// Game logic runs on Unity's main thread from a hook on Camera.FireOnPreCull (once per frame,
// right before rendering); the ImGui overlay talks to it through g_State as before.

#include "game.h"
#include "il2cpp.h"
#include "shared.h"

#include "../third_party/minhook/include/MinHook.h"

#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <deque>
#include <map>
#include <mutex>
#include <string>
#include <vector>

using il2::Call;
using il2::CallV;
using il2::Fn;

// ============================================================================ log

static std::mutex g_LogLock;
static std::deque<std::string> g_LogLines;
static bool g_LogChanged;
static std::string g_LogPath;

void Game_Log(const char* fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    std::lock_guard<std::mutex> lock(g_LogLock);
    g_LogLines.push_back(std::string("[Info   :Cluado] ") + buf);
    if (g_LogLines.size() > CONSOLE_LINES)
        g_LogLines.pop_front();
    g_LogChanged = true;

    if (!g_LogPath.empty())
        if (FILE* f = fopen(g_LogPath.c_str(), "a"))
        {
            fprintf(f, "%s\n", buf);
            fclose(f);
        }
}

static void FlushLogToState()
{
    std::lock_guard<std::mutex> lock(g_LogLock);
    if (!g_LogChanged)
        return;
    g_LogChanged = false;
    int i = 0;
    for (const auto& line : g_LogLines)
    {
        strncpy(g_State.console[i], line.c_str(), CONSOLE_LINE_LEN - 1);
        g_State.console[i][CONSOLE_LINE_LEN - 1] = 0;
        ++i;
    }
    g_State.consoleCount = i;
}

static void WriteStr(char* dst, size_t cap, const std::string& s)
{
    size_t n = std::min(cap - 1, s.size());
    memcpy(dst, s.data(), n);
    dst[n] = 0;
}

// ============================================================================ settings (Cluado.ini next to the DLL)

static std::string g_IniPath;

static void LoadSettings()
{
    // Defaults, same as the BepInEx version.
    g_State.fovEnabled = 1; g_State.fov = 90.0f;
    g_State.speedMult = 1.0f; g_State.noclipSpeed = 8.0f; g_State.timeScale = 1.0f;
    g_State.espBoxes = 1; g_State.espNames = 1; g_State.espDistance = 1;
    g_State.watermark = 1; g_State.notifications = 1; g_State.menuParticles = 1;
    g_State.theme = 52; g_State.orbitRadius = 3.0f; g_State.orbitSpeed = 1.0f;

    FILE* f = fopen(g_IniPath.c_str(), "r");
    if (!f)
        return;
    char line[256];
    while (fgets(line, sizeof(line), f))
    {
        char key[64];
        float v;
        if (sscanf(line, "%63[^=]=%f", key, &v) != 2)
            continue;
        std::string k = key;
#define INT_KEY(name) if (k == #name) g_State.name = (int32_t)v;
#define FLT_KEY(name) if (k == #name) g_State.name = v;
        INT_KEY(fovEnabled) FLT_KEY(fov) FLT_KEY(speedMult) FLT_KEY(noclipSpeed) FLT_KEY(timeScale)
        INT_KEY(espBoxes) INT_KEY(espLines) INT_KEY(espNames) INT_KEY(espDistance) INT_KEY(espCorner)
        INT_KEY(espFilled) INT_KEY(esp3D) INT_KEY(espHeadDot) INT_KEY(watermark) INT_KEY(crosshair)
        INT_KEY(notifications) INT_KEY(theme) INT_KEY(menuParticles) FLT_KEY(orbitRadius) FLT_KEY(orbitSpeed)
#undef INT_KEY
#undef FLT_KEY
    }
    fclose(f);
}

static void SaveSettings()
{
    FILE* f = fopen(g_IniPath.c_str(), "w");
    if (!f)
        return;
#define W_INT(name) fprintf(f, #name "=%d\n", (int)g_State.name);
#define W_FLT(name) fprintf(f, #name "=%g\n", (double)g_State.name);
    W_INT(fovEnabled) W_FLT(fov) W_FLT(speedMult) W_FLT(noclipSpeed) W_FLT(timeScale)
    W_INT(espBoxes) W_INT(espLines) W_INT(espNames) W_INT(espDistance) W_INT(espCorner)
    W_INT(espFilled) W_INT(esp3D) W_INT(espHeadDot) W_INT(watermark) W_INT(crosshair)
    W_INT(notifications) W_INT(theme) W_INT(menuParticles) W_FLT(orbitRadius) W_FLT(orbitSpeed)
#undef W_INT
#undef W_FLT
    fclose(f);
}

// ============================================================================ IL2CPP bindings

namespace U  // Unity
{
    void *Object, *Component, *Behaviour, *GameObject, *Transform, *Camera, *Time, *Screen, *Cursor,
         *RenderSettings, *PlayerPrefs, *Light, *Quaternion, *Application, *Input,
         *CharacterController, *Collider, *NavMeshAgent, *NavMesh;

    Fn FindObjectsOfType, GetName;
    Fn GetTransform, GetGameObject, GetComponentsInChildren;
    Fn GetEnabled, SetEnabled, IsActiveAndEnabled;
    Fn GoActive, GoSetActive, GoAddComponent, GoCtor, GoTransform;
    Fn GetPosition, SetPosition, GetForward, GetRight, SetRotation, TGetParent, TSetParent;
    Fn CamMain, CamAll, CamGetFov, CamSetFov, CamDepth, CamMask, CamTarget, CamRect, CamW2S, FireOnPreCull;
    Fn DeltaTime, UnscaledTime, FrameCount, SetTimeScale;
    Fn ScreenW, ScreenH;
    Fn SetLockState, SetVisible;
    Fn GetAmbientMode, SetAmbientMode, GetAmbientLight, SetAmbientLight, GetFog, SetFog;
    Fn PrefsGetFloat, PrefsSetFloat, PrefsGetInt, PrefsSetInt, PrefsHasKey, PrefsSave;
    Fn LightType, LightRange, LightIntensity, LightShadows;
    Fn LookRotation, CompanyName, ProductName;
    Fn GetKey;
    Fn CcMove, CcGrounded, ColBounds, ColGetEnabled, ColSetEnabled;
    Fn AgentOnNavMesh, AgentSetStopped, AgentWarp, AgentHeight, SamplePosition;
}

namespace G  // Game (Assembly-CSharp)
{
    void *MobileFPS, *PlayerStatus, *Days, *Inventory, *ItemDefs, *EnemyController, *MainMenu, *ColorToyCapsule;
    void *Eyes_Granny, *Eyes_Grandpa, *Eyes_MomSpider;
    void *AI_Granny, *AI_Grandpa;

    int fpsCc = -1, fpsCam2 = -1, fpsCam = -1, fpsPS = -1;
    int psPlayerCam = -1;
    int psHide[9];
    int daysCurrentDay = -1;
    int invItemDefs = -1, itemName = -1;
    int eyesCam = -1;

    const void *GrannyDeath, *GrandpaDeath, *PickupItem, *PepperGranny, *PepperGrandpa, *IceGranny, *IceGrandpa;
    const void* UnlockCertainSkin;
    std::vector<const void*> AttemptBuys;
}

struct EnemyType
{
    const char* cls;
    const char* agentField;
    const char* label;
    int kind;
    void* klass = nullptr;
    int agentOff = -1;
};

static EnemyType g_EnemyTypes[] = {
    { "AI_Granny", "Agent", "Granny", Esp_Granny },
    { "AI_Grandpa", "Agent", "Grandpa", Esp_Grandpa },
    { "AI_Slendrina", "Agent", "Slendrina", Esp_Slendrina },
    { "AI_MomSpider", "Agent", "Spider", Esp_Spider },
    { "AI_RobotEnemy", "Agent", "Robot", Esp_Robot },
    { "SpectralGranny", "Agent", "Spectral Granny", Esp_Granny },
    { "SunnyGrannyEnemy", nullptr, "Sunny Granny", Esp_Granny },
    { "LittleFucker", "Agent", "Creature", Esp_Other },
    { "LeprechaunAI", "agent", "Leprechaun", Esp_Other },
    { "SnowManAI", "Agent", "Snowman", Esp_Other },
    { "RatEnemy", "Agent", "Rat", Esp_Other },
    { "UndeadBunny", "agent", "Undead Bunny", Esp_Other },
};

static void* CS(const char* name) { return il2::Class("Assembly-CSharp.dll", "", name); }
static void* UC(const char* image, const char* ns, const char* name) { return il2::Class(image, ns, name); }

static bool Resolve()
{
    const char* core = "UnityEngine.CoreModule.dll";
    using namespace U;
    Object = UC(core, "UnityEngine", "Object");
    Component = UC(core, "UnityEngine", "Component");
    Behaviour = UC(core, "UnityEngine", "Behaviour");
    GameObject = UC(core, "UnityEngine", "GameObject");
    Transform = UC(core, "UnityEngine", "Transform");
    Camera = UC(core, "UnityEngine", "Camera");
    Time = UC(core, "UnityEngine", "Time");
    Screen = UC(core, "UnityEngine", "Screen");
    Cursor = UC(core, "UnityEngine", "Cursor");
    RenderSettings = UC(core, "UnityEngine", "RenderSettings");
    PlayerPrefs = UC(core, "UnityEngine", "PlayerPrefs");
    Light = UC(core, "UnityEngine", "Light");
    Quaternion = UC(core, "UnityEngine", "Quaternion");
    Application = UC(core, "UnityEngine", "Application");
    Input = UC("UnityEngine.InputLegacyModule.dll", "UnityEngine", "Input");
    CharacterController = UC("UnityEngine.PhysicsModule.dll", "UnityEngine", "CharacterController");
    Collider = UC("UnityEngine.PhysicsModule.dll", "UnityEngine", "Collider");
    NavMeshAgent = UC("UnityEngine.AIModule.dll", "UnityEngine.AI", "NavMeshAgent");
    NavMesh = UC("UnityEngine.AIModule.dll", "UnityEngine.AI", "NavMesh");

    FindObjectsOfType = il2::Func(Object, "FindObjectsOfType", 1);
    GetName = il2::Func(Object, "get_name", 0);
    GetTransform = il2::Func(Component, "get_transform", 0);
    GetGameObject = il2::Func(Component, "get_gameObject", 0);
    GetComponentsInChildren = il2::Func(Component, "GetComponentsInChildren", 2);
    GetEnabled = il2::Func(Behaviour, "get_enabled", 0);
    SetEnabled = il2::Func(Behaviour, "set_enabled", 1);
    IsActiveAndEnabled = il2::Func(Behaviour, "get_isActiveAndEnabled", 0);
    GoActive = il2::Func(GameObject, "get_activeInHierarchy", 0);
    GoSetActive = il2::Func(GameObject, "SetActive", 1);
    GoAddComponent = il2::Func(GameObject, "AddComponent", 1);
    GoCtor = il2::Func(GameObject, ".ctor", 1);
    GoTransform = il2::Func(GameObject, "get_transform", 0);
    GetPosition = il2::Func(Transform, "get_position", 0);
    SetPosition = il2::Func(Transform, "set_position", 1);
    GetForward = il2::Func(Transform, "get_forward", 0);
    GetRight = il2::Func(Transform, "get_right", 0);
    SetRotation = il2::Func(Transform, "set_rotation", 1);
    TGetParent = il2::Func(Transform, "get_parent", 0);
    TSetParent = il2::Func(Transform, "SetParent", 2);
    CamMain = il2::Func(Camera, "get_main", 0);
    CamAll = il2::Func(Camera, "get_allCameras", 0);
    CamGetFov = il2::Func(Camera, "get_fieldOfView", 0);
    CamSetFov = il2::Func(Camera, "set_fieldOfView", 1);
    CamDepth = il2::Func(Camera, "get_depth", 0, "UnityEngine.Camera::get_depth");
    CamMask = il2::Func(Camera, "get_cullingMask", 0);
    CamTarget = il2::Func(Camera, "get_targetTexture", 0);
    CamRect = il2::Func(Camera, "get_rect", 0);
    CamW2S = il2::Func(Camera, "WorldToScreenPoint", 1);
    FireOnPreCull = il2::Func(Camera, "FireOnPreCull", 1);
    DeltaTime = il2::Func(Time, "get_deltaTime", 0);
    UnscaledTime = il2::Func(Time, "get_unscaledTime", 0);
    FrameCount = il2::Func(Time, "get_frameCount", 0, "UnityEngine.Time::get_frameCount");
    SetTimeScale = il2::Func(Time, "set_timeScale", 1);
    ScreenW = il2::Func(Screen, "get_width", 0);
    ScreenH = il2::Func(Screen, "get_height", 0);
    SetLockState = il2::Func(Cursor, "set_lockState", 1, "UnityEngine.Cursor::set_lockState");
    SetVisible = il2::Func(Cursor, "set_visible", 1, "UnityEngine.Cursor::set_visible");
    GetAmbientMode = il2::Func(RenderSettings, "get_ambientMode", 0);
    SetAmbientMode = il2::Func(RenderSettings, "set_ambientMode", 1);
    GetAmbientLight = il2::Func(RenderSettings, "get_ambientLight", 0);
    SetAmbientLight = il2::Func(RenderSettings, "set_ambientLight", 1);
    GetFog = il2::Func(RenderSettings, "get_fog", 0);
    SetFog = il2::Func(RenderSettings, "set_fog", 1);
    PrefsGetFloat = il2::Func(PlayerPrefs, "GetFloat", 2);
    PrefsSetFloat = il2::Func(PlayerPrefs, "SetFloat", 2);
    PrefsGetInt = il2::Func(PlayerPrefs, "GetInt", 2);
    PrefsSetInt = il2::Func(PlayerPrefs, "SetInt", 2);
    PrefsHasKey = il2::Func(PlayerPrefs, "HasKey", 1);
    PrefsSave = il2::Func(PlayerPrefs, "Save", 0);
    LightType = il2::Func(Light, "set_type", 1, "UnityEngine.Light::set_type");
    LightRange = il2::Func(Light, "set_range", 1, "UnityEngine.Light::set_range");
    LightIntensity = il2::Func(Light, "set_intensity", 1, "UnityEngine.Light::set_intensity");
    LightShadows = il2::Func(Light, "set_shadows", 1, "UnityEngine.Light::set_shadows");
    LookRotation = il2::Func(Quaternion, "LookRotation", 1);
    CompanyName = il2::Func(Application, "get_companyName", 0, "UnityEngine.Application::get_companyName");
    ProductName = il2::Func(Application, "get_productName", 0, "UnityEngine.Application::get_productName");
    GetKey = il2::Func(Input, "GetKey", 1);
    CcMove = il2::Func(CharacterController, "Move", 1);
    CcGrounded = il2::Func(CharacterController, "get_isGrounded", 0);
    ColBounds = il2::Func(Collider, "get_bounds", 0);
    ColGetEnabled = il2::Func(Collider, "get_enabled", 0);
    ColSetEnabled = il2::Func(Collider, "set_enabled", 1);
    AgentOnNavMesh = il2::Func(NavMeshAgent, "get_isOnNavMesh", 0);
    AgentSetStopped = il2::Func(NavMeshAgent, "set_isStopped", 1);
    AgentWarp = il2::Func(NavMeshAgent, "Warp", 1);
    AgentHeight = il2::Func(NavMeshAgent, "get_height", 0, "UnityEngine.AI.NavMeshAgent::get_height");
    SamplePosition = il2::Func(NavMesh, "SamplePosition", 4);

    using namespace G;
    MobileFPS = CS("MobileFPS");
    PlayerStatus = CS("PlayerStatus");
    Days = CS("Days");
    Inventory = CS("Inventory");
    ItemDefs = CS("ItemDefs");
    EnemyController = CS("EnemyController");
    MainMenu = CS("MainMenu");
    ColorToyCapsule = CS("ColorToyCapsule");
    Eyes_Granny = CS("Eyes_Granny");
    Eyes_Grandpa = CS("Eyes_Grandpa");
    Eyes_MomSpider = CS("Eyes_MomSpider");
    AI_Granny = CS("AI_Granny");
    AI_Grandpa = CS("AI_Grandpa");

    fpsCc = il2::Field(MobileFPS, "characterController");
    fpsCam2 = il2::Field(MobileFPS, "playerCamera2");
    fpsCam = il2::Field(MobileFPS, "playerCamera");
    fpsPS = il2::Field(MobileFPS, "PS");
    psPlayerCam = il2::Field(PlayerStatus, "PlayerCam");
    const char* hideFlags[9] = { "HidingUnderThings", "HidingInClosets", "HidingFromMom", "InBed1", "InBed2",
                                 "InBed3", "InCar", "InCoffinBackyard", "InCoffinAttic" };
    for (int i = 0; i < 9; ++i)
        psHide[i] = il2::Field(PlayerStatus, hideFlags[i]);
    daysCurrentDay = il2::Field(Days, "CurrentDay");
    invItemDefs = il2::Field(Inventory, "ItemDefs");
    itemName = il2::Field(ItemDefs, "itemName");
    eyesCam = il2::Field(Eyes_Granny, "CameraView");

    GrannyDeath = il2::Method(EnemyController, "GrannyDeath", 1);
    GrandpaDeath = il2::Method(EnemyController, "GrandpaDeath", 1);
    PickupItem = il2::Method(Inventory, "PickupItem", 1);
    PepperGranny = il2::Method(AI_Granny, "PepperHit", 0);
    PepperGrandpa = il2::Method(AI_Grandpa, "PepperHit", 0);
    IceGranny = il2::Method(AI_Granny, "FrozenEnemy", 0);
    IceGrandpa = il2::Method(AI_Grandpa, "FrozenEnemy", 0);
    UnlockCertainSkin = il2::Method(ColorToyCapsule, "UnlockCertainSkin", 0);
    for (const char* n : { "AttemptBuy_NM", "AttemptBuy_GR", "AttemptBuy_SH", "AttemptBuy_IM", "AttemptBuy_EX",
                           "AttemptBuy_CC", "AttemptBuy_CH", "AttemptBuy_TRAP", "AttemptBuy_EXTRAPS",
                           "AttemptBuy_ROBO", "AttemptBuy_FLASH", "AttemptBuy_VERC", "AttemptBuy_HO" })
        if (const void* m = il2::Method(MainMenu, n, 0))
            AttemptBuys.push_back(m);

    for (auto& t : g_EnemyTypes)
    {
        t.klass = CS(t.cls);
        t.agentOff = t.agentField ? il2::Field(t.klass, t.agentField) : -1;
    }

    bool ok = FindObjectsOfType && GetTransform && GetPosition && FireOnPreCull && MobileFPS && fpsCc >= 0;
    Game_Log("Resolve: %s (FindObjectsOfType %d, FireOnPreCull %d, MobileFPS %d, W2S %d, frameCount %d)",
             ok ? "ok" : "MISSING PIECES", !!FindObjectsOfType, !!FireOnPreCull, !!MobileFPS, !!CamW2S, !!FrameCount);
    return ok;
}

// ============================================================================ helpers

static bool Alive(void* o) { return il2::Alive(o); }

static std::vector<void*> FindAll(void* klass)
{
    std::vector<void*> out;
    if (!klass)
        return out;
    void* arr = Call<void*>(U::FindObjectsOfType, il2::TypeObject(klass));
    uintptr_t n = il2::ArrayLength(arr);
    void** data = il2::ArrayData(arr);
    for (uintptr_t i = 0; i < n; ++i)
        if (Alive(data[i]))
            out.push_back(data[i]);
    return out;
}

static void* FindOne(void* klass)
{
    auto all = FindAll(klass);
    return all.empty() ? nullptr : all[0];
}

static void* TransformOf(void* comp) { return Alive(comp) ? Call<void*>(U::GetTransform, comp) : nullptr; }
static void* GameObjectOf(void* comp) { return Alive(comp) ? Call<void*>(U::GetGameObject, comp) : nullptr; }
static bool ActiveGO(void* comp)
{
    void* go = GameObjectOf(comp);
    return go && Call<bool>(U::GoActive, go);
}
static Vec3 PosOf(void* comp)
{
    void* t = TransformOf(comp);
    return t ? Call<Vec3>(U::GetPosition, t) : Vec3{};
}
static Vec3 Add(Vec3 a, Vec3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
static Vec3 Sub(Vec3 a, Vec3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
static Vec3 Mul(Vec3 a, float s) { return { a.x * s, a.y * s, a.z * s }; }
static float Len(Vec3 a) { return sqrtf(a.x * a.x + a.y * a.y + a.z * a.z); }
static Vec3 Norm(Vec3 a) { float l = Len(a); return l > 1e-5f ? Mul(a, 1.0f / l) : Vec3{}; }
static std::string Lower(std::string s) { for (auto& c : s) c = (char)tolower((unsigned char)c); return s; }

static bool g_QuietPrefs;  // our own PlayerPrefs reads don't count for coin detection
static float PrefsGetFloatQ(const char* key, float def)
{
    g_QuietPrefs = true;
    float v = Call<float>(U::PrefsGetFloat, il2::Str(key), def);
    g_QuietPrefs = false;
    return v;
}
static int PrefsGetIntQ(const char* key, int def)
{
    g_QuietPrefs = true;
    int v = Call<int>(U::PrefsGetInt, il2::Str(key), def);
    g_QuietPrefs = false;
    return v;
}

// ============================================================================ scene state

struct Enemy
{
    void* ai;
    void* agent;
    void* head;
    const char* label;
    int kind;
    int id;
};

static void* g_Fps;
static void* g_Cc;
static void* g_Days;
static void* g_Inventory;
static void* g_EnemyCtl;
static void* g_MainMenu;
static void* g_Capsule;
static void* g_PlayerCam;
static std::vector<Enemy> g_Enemies;
static std::vector<void*> g_EnemyEyes;
static std::vector<std::string> g_Items;
static std::map<void*, void*> g_HeadCache;
static float g_NextRefresh;

static int IdOf(void* ai)
{
    uintptr_t p = (uintptr_t)ai;
    return (int)(p ^ (p >> 32));
}

static void* FindHead(void* ai)
{
    void* arr = Call<void*>(U::GetComponentsInChildren, ai, il2::TypeObject(U::Transform), true);
    uintptr_t n = il2::ArrayLength(arr);
    void** data = il2::ArrayData(arr);
    for (uintptr_t i = 0; i < n; ++i)
    {
        if (!Alive(data[i]))
            continue;
        std::string name = Lower(il2::Utf8(Call<void*>(U::GetName, data[i])));
        if (name.find("head") != std::string::npos && name.find("end") == std::string::npos &&
            name.find("top") == std::string::npos && name.find("nub") == std::string::npos)
            return data[i];
    }
    return nullptr;
}

static void RefreshScene()
{
    // Player: prefer the MobileFPS whose controller is live.
    g_Fps = nullptr;
    void* first = nullptr;
    for (void* fps : FindAll(G::MobileFPS))
    {
        if (!first) first = fps;
        void* cc = il2::Get<void*>(fps, G::fpsCc);
        if (Alive(cc) && Call<bool>(U::ColGetEnabled, cc) && ActiveGO(cc))
        {
            g_Fps = fps;
            break;
        }
    }
    if (!g_Fps) g_Fps = first;
    g_Cc = g_Fps ? il2::Get<void*>(g_Fps, G::fpsCc) : nullptr;
    if (!Alive(g_Cc)) g_Cc = nullptr;

    g_Days = FindOne(G::Days);
    g_Inventory = FindOne(G::Inventory);
    g_EnemyCtl = FindOne(G::EnemyController);
    g_MainMenu = FindOne(G::MainMenu);
    g_Capsule = FindOne(G::ColorToyCapsule);

    g_Enemies.clear();
    if (g_HeadCache.size() > 256)
        g_HeadCache.clear();
    for (auto& t : g_EnemyTypes)
    {
        for (void* ai : FindAll(t.klass))
        {
            Enemy e{};
            e.ai = ai;
            e.agent = t.agentOff >= 0 ? il2::Get<void*>(ai, t.agentOff) : nullptr;
            if (!Alive(e.agent)) e.agent = nullptr;
            e.label = t.label;
            e.kind = t.kind;
            e.id = IdOf(ai);
            auto it = g_HeadCache.find(ai);
            if (it == g_HeadCache.end() || (it->second && !Alive(it->second)))
                it = g_HeadCache.insert_or_assign(ai, FindHead(ai)).first;
            e.head = it->second;
            g_Enemies.push_back(e);
        }
    }

    g_EnemyEyes.clear();
    for (void* k : { G::Eyes_Granny, G::Eyes_Grandpa, G::Eyes_MomSpider })
        for (void* eyes : FindAll(k))
        {
            int off = il2::Field(il2::ClassOf(eyes), "CameraView");
            if (void* cam = il2::Get<void*>(eyes, off))
                g_EnemyEyes.push_back(cam);
        }

    g_Items.clear();
    if (g_Inventory)
    {
        void* list = il2::Get<void*>(g_Inventory, G::invItemDefs);
        if (list)
        {
            void* listClass = il2::ClassOf(list);
            void* items = il2::Get<void*>(list, il2::Field(listClass, "_items"));
            int size = il2::Get<int>(list, il2::Field(listClass, "_size"));
            void** data = il2::ArrayData(items);
            for (int i = 0; items && i < size && i < (int)il2::ArrayLength(items); ++i)
            {
                std::string name = il2::Utf8(il2::Get<void*>(data[i], G::itemName));
                if (!name.empty() && std::find(g_Items.begin(), g_Items.end(), name) == g_Items.end())
                    g_Items.push_back(name);
            }
        }
    }
}

// ============================================================================ camera / FOV

static void ResolveCamera()
{
    void* own = nullptr;
    if (g_Fps)
    {
        void* c = il2::Get<void*>(g_Fps, G::fpsCam2);
        if (Alive(c) && Call<bool>(U::IsActiveAndEnabled, c)) own = c;
        if (!own)
        {
            void* ps = il2::Get<void*>(g_Fps, G::fpsPS);
            void* pc = Alive(ps) ? il2::Get<void*>(ps, G::psPlayerCam) : nullptr;
            if (Alive(pc) && Call<bool>(U::IsActiveAndEnabled, pc)) own = pc;
        }
    }

    // The camera drawing the world last: on screen, renders Default, highest depth, not an enemy's eyes.
    void* best = nullptr;
    float bestDepth = -1e9f;
    {
        void* arr = U::CamAll ? Call<void*>(U::CamAll) : Call<void*>(U::FindObjectsOfType, il2::TypeObject(U::Camera));
        uintptr_t n = il2::ArrayLength(arr);
        void** data = il2::ArrayData(arr);
        for (uintptr_t i = 0; i < n; ++i)
        {
            void* c = data[i];
            if (!Alive(c) || !Call<bool>(U::IsActiveAndEnabled, c))
                continue;
            if (Call<void*>(U::CamTarget, c) || (Call<int>(U::CamMask, c) & 1) == 0)
                continue;
            if (std::find(g_EnemyEyes.begin(), g_EnemyEyes.end(), c) != g_EnemyEyes.end())
                continue;
            Rect r = Call<Rect>(U::CamRect, c);
            if (r.w * r.h < 0.5f)
                continue;
            float d = Call<float>(U::CamDepth, c);
            if (!best || d > bestDepth || (d == bestDepth && c == own))
            {
                best = c;
                bestDepth = d;
            }
        }
    }
    g_PlayerCam = best ? best : own;
    if (!g_PlayerCam)
    {
        void* m = Call<void*>(U::CamMain);
        g_PlayerCam = Alive(m) ? m : nullptr;
    }
}

static void ApplyFov(void* cam)
{
    if (g_State.fovEnabled && cam && cam == g_PlayerCam && Alive(cam))
        CallV(U::CamSetFov, cam, g_State.fov);
}

// ============================================================================ cheats

static std::vector<std::pair<void*, void*>> g_Frozen;  // global freeze
static std::vector<std::pair<void*, void*>> g_FrozenOne;
static std::vector<void*> g_Despawned;

static void FreezeOne(void* ai, void* agent, std::vector<std::pair<void*, void*>>& list)
{
    if (!Alive(ai) || !Call<bool>(U::GetEnabled, ai))
        return;
    CallV(U::SetEnabled, ai, false);
    if (Alive(agent) && Call<bool>(U::GetEnabled, agent) && Call<bool>(U::AgentOnNavMesh, agent))
        CallV(U::AgentSetStopped, agent, true);
    list.push_back({ ai, agent });
}

static void Unfreeze(std::vector<std::pair<void*, void*>>& list, void* only = nullptr)
{
    for (auto it = list.begin(); it != list.end();)
    {
        if (only && it->first != only) { ++it; continue; }
        if (Alive(it->first)) CallV(U::SetEnabled, it->first, true);
        if (Alive(it->second) && Call<bool>(U::GetEnabled, it->second) && Call<bool>(U::AgentOnNavMesh, it->second))
            CallV(U::AgentSetStopped, it->second, false);
        it = list.erase(it);
    }
}

static void ApplyFreeze()
{
    if (!g_State.freezeEnemies)
    {
        if (!g_Frozen.empty()) Unfreeze(g_Frozen);
        return;
    }
    for (auto& e : g_Enemies)
        FreezeOne(e.ai, e.agent, g_Frozen);
}

// Speed: add (mult-1) x how far the game moved the player this frame.
static Vec3 g_LastPos;
static void* g_LastCc;

static bool PlayerWalking()
{
    if (!g_Cc || !Call<bool>(U::ColGetEnabled, g_Cc) || !ActiveGO(g_Cc) || !Call<bool>(U::GetEnabled, g_Fps))
        return false;
    void* ps = il2::Get<void*>(g_Fps, G::fpsPS);
    if (!Alive(ps))
        return true;
    for (int off : G::psHide)
        if (il2::Get<bool>(ps, off))
            return false;
    return true;
}

static void ApplySpeed()
{
    if (!g_Cc)
    {
        g_LastCc = nullptr;
        return;
    }
    void* t = TransformOf(g_Cc);
    if (!t) return;
    Vec3 pos = Call<Vec3>(U::GetPosition, t);
    bool same = g_LastCc == g_Cc;
    g_LastCc = g_Cc;

    if (same && !g_State.noclip && fabsf(g_State.speedMult - 1.0f) > 0.01f && PlayerWalking())
    {
        float dx = pos.x - g_LastPos.x, dz = pos.z - g_LastPos.z;
        float moved = dx * dx + dz * dz;
        if (moved > 1e-10f && moved < 1.0f)
        {
            float extra = g_State.speedMult - 1.0f;
            float down = Call<bool>(U::CcGrounded, g_Cc) ? -(sqrtf(moved) * fabsf(extra) + 0.05f) : 0.0f;
            Call<int>(U::CcMove, g_Cc, Vec3{ dx * extra, down, dz * extra });
            pos = Call<Vec3>(U::GetPosition, t);
        }
    }
    g_LastPos = pos;
}

static bool g_WasNoclip;
static void ApplyNoclip()
{
    if (!g_Cc) return;
    if (!g_State.noclip)
    {
        if (g_WasNoclip) CallV(U::ColSetEnabled, g_Cc, true);
        g_WasNoclip = false;
        return;
    }
    g_WasNoclip = true;
    CallV(U::ColSetEnabled, g_Cc, false);

    void* view = il2::Get<void*>(g_Fps, G::fpsCam);
    if (!Alive(view)) view = g_PlayerCam ? TransformOf(g_PlayerCam) : nullptr;
    if (!view) return;

    Vec3 f = Call<Vec3>(U::GetForward, view), r = Call<Vec3>(U::GetRight, view), dir{};
    auto key = [](int k) { return Call<bool>(U::GetKey, k); };
    if (key(119)) dir = Add(dir, f);            // W
    if (key(115)) dir = Sub(dir, f);            // S
    if (key(100)) dir = Add(dir, r);            // D
    if (key(97)) dir = Sub(dir, r);             // A
    if (key(32)) dir.y += 1.0f;                 // Space
    if (key(306)) dir.y -= 1.0f;                // LeftControl
    float speed = g_State.noclipSpeed * (key(304) ? 3.0f : 1.0f);  // LeftShift
    void* t = TransformOf(g_Cc);
    Vec3 p = Call<Vec3>(U::GetPosition, t);
    CallV(U::SetPosition, t, Add(p, Mul(Norm(dir), speed * Call<float>(U::DeltaTime))));
}

static bool g_WasTimeScale;
static void ApplyTimeScale()
{
    if (g_State.timeScaleEnabled)
    {
        CallV(U::SetTimeScale, g_State.timeScale);
        g_WasTimeScale = true;
    }
    else if (g_WasTimeScale)
    {
        CallV(U::SetTimeScale, 1.0f);
        g_WasTimeScale = false;
    }
}

static bool g_BrightSaved;
static int g_AmbientMode;
static Color g_AmbientLight;
static bool g_Fog;
static void* g_Lamp;      // GameObject
static void* g_LampParent;
static void ApplyFullbright()
{
    if (g_State.fullbright)
    {
        if (!g_BrightSaved)
        {
            g_AmbientMode = Call<int>(U::GetAmbientMode);
            g_AmbientLight = Call<Color>(U::GetAmbientLight);
            g_Fog = Call<bool>(U::GetFog);
            g_BrightSaved = true;
        }
        CallV(U::SetAmbientMode, 3);  // Flat
        CallV(U::SetAmbientLight, Color{ 1, 1, 1, 1 });
        CallV(U::SetFog, false);

        if (!Alive(g_Lamp) && g_PlayerCam && U::GoCtor)
        {
            g_Lamp = il2::New(U::GameObject);
            CallV(U::GoCtor, g_Lamp, il2::Str("Cluado_Lamp"));
            void* light = Call<void*>(U::GoAddComponent, g_Lamp, il2::TypeObject(U::Light));
            if (light)
            {
                CallV(U::LightType, light, 2);  // Point
                CallV(U::LightRange, light, 40.0f);
                CallV(U::LightIntensity, light, 1.5f);
                CallV(U::LightShadows, light, 0);
            }
            g_LampParent = nullptr;
        }
        if (Alive(g_Lamp))
        {
            void* camT = g_PlayerCam ? TransformOf(g_PlayerCam) : nullptr;
            if (camT && camT != g_LampParent)
            {
                void* lampT = Call<void*>(U::GoTransform, g_Lamp);
                CallV(U::TSetParent, lampT, camT, false);
                g_LampParent = camT;
            }
            CallV(U::GoSetActive, g_Lamp, true);
        }
    }
    else if (g_BrightSaved)
    {
        CallV(U::SetAmbientMode, g_AmbientMode);
        CallV(U::SetAmbientLight, g_AmbientLight);
        CallV(U::SetFog, g_Fog);
        g_BrightSaved = false;
        if (Alive(g_Lamp)) CallV(U::GoSetActive, g_Lamp, false);
    }
}

// Orbit: enemies frozen and circling the player.
struct Rider { void* ai; void* agent; bool aiOn, agentOn; };
static std::map<void*, Rider> g_Riders;
static float g_OrbitAngle;
static void ApplyOrbit()
{
    if (!g_State.orbitEnabled)
    {
        for (auto& [k, r] : g_Riders)
        {
            if (Alive(r.agent) && r.agentOn)
            {
                CallV(U::SetEnabled, r.agent, true);
                if (Call<bool>(U::AgentOnNavMesh, r.agent))
                    Call<bool>(U::AgentWarp, r.agent, PosOf(r.ai));
            }
            if (Alive(r.ai) && r.aiOn) CallV(U::SetEnabled, r.ai, true);
        }
        g_Riders.clear();
        return;
    }
    if (!g_Cc) return;
    Bounds b = Call<Bounds>(U::ColBounds, g_Cc);
    Vec3 centre = PosOf(g_Cc);
    centre.y = b.center.y - b.extents.y;

    std::vector<Enemy*> active;
    for (auto& e : g_Enemies)
        if (Alive(e.ai) && ActiveGO(e.ai)) active.push_back(&e);
    if (active.empty()) return;

    g_OrbitAngle += g_State.orbitSpeed * Call<float>(U::DeltaTime);
    for (size_t i = 0; i < active.size(); ++i)
    {
        Enemy& e = *active[i];
        if (!g_Riders.count(e.ai))
        {
            Rider r{ e.ai, e.agent, Call<bool>(U::GetEnabled, e.ai), e.agent && Call<bool>(U::GetEnabled, e.agent) };
            CallV(U::SetEnabled, e.ai, false);
            if (e.agent) CallV(U::SetEnabled, e.agent, false);
            g_Riders[e.ai] = r;
        }
        float a = g_OrbitAngle + i * (6.2831853f / active.size());
        Vec3 pos = Add(centre, Vec3{ cosf(a) * g_State.orbitRadius, 0, sinf(a) * g_State.orbitRadius });
        void* t = TransformOf(e.ai);
        CallV(U::SetPosition, t, pos);
        Vec3 look = Sub(centre, pos);
        look.y = 0;
        if (Len(look) > 0.03f)
            CallV(U::SetRotation, t, Call<Quat>(U::LookRotation, look));
    }
}

// ============================================================================ shop

struct PrefRead { bool isFloat; int reads; bool inShop; };
static std::map<std::string, PrefRead> g_Prefs;
static int g_ShopDepth;
static std::string g_CoinKey;
static bool g_CoinFloat = true, g_CoinConfirmed, g_CoinRepaired;
static float g_Coins;
static float g_NextRegistry;

static bool Contains(const std::string& s, std::initializer_list<const char*> words)
{
    std::string l = Lower(s);
    for (const char* w : words)
        if (l.find(w) != std::string::npos) return true;
    return false;
}

static void RecordPref(void* keyStr, bool isFloat)
{
    if (g_QuietPrefs || !keyStr) return;
    std::string key = il2::Utf8(keyStr);
    if (key.empty()) return;
    auto& p = g_Prefs[key];
    p.isFloat = isFloat;
    p.reads++;
    if (g_ShopDepth > 0 && !p.inShop)
    {
        p.inShop = true;
        Game_Log("Shop reads PlayerPrefs \"%s\" as %s", key.c_str(), isFloat ? "float" : "int");
    }
}

static void PickCoinKey()
{
    const std::pair<const std::string, PrefRead>* best = nullptr;
    auto better = [](const auto* a, const auto& b, bool preferShop) {
        if (!a) return true;
        bool ac = Contains(a->first, { "coin", "money", "cash", "currency" }), bc = Contains(b.first, { "coin", "money", "cash", "currency" });
        if (preferShop && ac != bc) return bc;
        if (!preferShop && a->second.inShop != b.second.inShop) return b.second.inShop;
        return b.second.reads > a->second.reads;
    };
    for (auto& kv : g_Prefs)
        if (kv.second.inShop && kv.second.isFloat && !Contains(kv.first, { "volume", "sens", "fov", "bright", "gamma", "quality", "music", "sound" }))
            if (better(best, kv, true)) best = &kv;
    if (!best)
        for (auto& kv : g_Prefs)
            if (Contains(kv.first, { "coin", "money", "cash", "currency" }))
                if (better(best, kv, false)) best = &kv;
    if (!best) return;
    if (!g_CoinConfirmed || g_CoinKey != best->first || g_CoinFloat != best->second.isFloat)
    {
        g_CoinKey = best->first;
        g_CoinFloat = best->second.isFloat;
        g_CoinConfirmed = true;
        g_CoinRepaired = false;
        Game_Log("Coins: the game reads \"%s\" as %s", g_CoinKey.c_str(), g_CoinFloat ? "float" : "int");
    }
}

static void GuessCoinKeyFromRegistry()
{
    std::string company = il2::Utf8(Call<void*>(U::CompanyName));
    std::string product = il2::Utf8(Call<void*>(U::ProductName));
    std::string path = "Software\\" + company + "\\" + product;
    HKEY key;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, path.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS)
        return;
    char name[256];
    for (DWORD i = 0;; ++i)
    {
        DWORD len = sizeof(name), type = 0;
        if (RegEnumValueA(key, i, name, &len, nullptr, &type, nullptr, nullptr) != ERROR_SUCCESS)
            break;
        std::string n(name, len);
        size_t h = n.rfind("_h");
        if (h != std::string::npos) n = n.substr(0, h);
        if (!Contains(n, { "coin", "money", "cash", "currency" }) || type == REG_BINARY)
            continue;
        g_CoinKey = n;
        g_CoinFloat = true;  // shop prices are fractions (0.5)
        Game_Log("Coins (guess from registry): \"%s\"", n.c_str());
        break;
    }
    RegCloseKey(key);
}

static void RefreshShop(float now)
{
    PickCoinKey();
    if (!g_CoinConfirmed && now >= g_NextRegistry)
    {
        g_NextRegistry = now + 5.0f;
        GuessCoinKeyFromRegistry();
    }
    if (g_CoinKey.empty()) return;

    if (!g_CoinRepaired && g_CoinConfirmed && g_CoinFloat)
    {
        g_CoinRepaired = true;
        int asInt = PrefsGetIntQ(g_CoinKey.c_str(), INT32_MIN);
        if (asInt != INT32_MIN && PrefsGetFloatQ(g_CoinKey.c_str(), -1.0f) == -1.0f)
        {
            CallV(U::PrefsSetFloat, il2::Str(g_CoinKey.c_str()), (float)std::clamp(asInt, 0, 9999999));
            CallV(U::PrefsSave);
            Game_Log("Coins: repaired \"%s\" (saved as a whole number by an older version)", g_CoinKey.c_str());
        }
    }
    g_Coins = g_CoinFloat ? PrefsGetFloatQ(g_CoinKey.c_str(), 0.0f) : (float)PrefsGetIntQ(g_CoinKey.c_str(), 0);
}

static void AddCoins(float amount)
{
    if (g_CoinKey.empty())
    {
        Game_Log("Coins not found yet - open the main menu or the toy capsule shop once.");
        return;
    }
    float next = std::max(g_Coins, std::min(std::max(g_Coins, 0.0f) + amount, 9999999.0f));
    if (g_CoinFloat) CallV(U::PrefsSetFloat, il2::Str(g_CoinKey.c_str()), next);
    else CallV(U::PrefsSetInt, il2::Str(g_CoinKey.c_str()), (int)next);
    CallV(U::PrefsSave);
    g_Coins = next;
    Game_Log("Coins: %.1f", next);
}

static void UnlockEverything()
{
    if (!g_MainMenu) { Game_Log("Unlock everything only works in the main menu."); return; }
    int done = 0;
    for (const void* m : G::AttemptBuys)
    {
        AddCoins(100000);
        bool failed = false;
        il2::Invoke(m, g_MainMenu, nullptr, &failed);
        if (!failed) ++done;
    }
    Game_Log("Unlock everything: ran %d/%d shop purchases. Restart the main menu to see them.", done, (int)G::AttemptBuys.size());
}

static void UnlockAllColours()
{
    if (!g_Capsule) { Game_Log("Stand at the toy capsule machine first."); return; }
    for (int i = 0; i < 300; ++i)
        il2::Invoke(G::UnlockCertainSkin, g_Capsule, nullptr);
    Game_Log("Toy capsule: rolled 300 times.");
}

// ============================================================================ actions

static void KnockOut(bool granny)
{
    if (!g_EnemyCtl) return;
    bool poisoned = false;
    void* args[1] = { &poisoned };
    il2::Invoke(granny ? G::GrannyDeath : G::GrandpaDeath, g_EnemyCtl, args);
}

static void MoveEnemy(void* ai, void* agent, Vec3 where)
{
    NavMeshHit hit{};
    if (Call<bool>(U::SamplePosition, where, &hit, 25.0f, -1))
        where = hit.position;
    if (Alive(agent) && Call<bool>(U::GetEnabled, agent) && Call<bool>(U::AgentOnNavMesh, agent))
        Call<bool>(U::AgentWarp, agent, where);
    else if (void* t = TransformOf(ai))
        CallV(U::SetPosition, t, where);
}

static void RunWheel(int action, int target)
{
    if (action == 10)  // Respawn all
    {
        for (void* go : g_Despawned)
            if (Alive(go)) CallV(U::GoSetActive, go, true);
        g_Despawned.clear();
        return;
    }
    Enemy* e = nullptr;
    for (auto& x : g_Enemies)
        if (x.id == target && Alive(x.ai)) e = &x;
    if (!e) return;

    void* klass = il2::ClassOf(e->ai);
    bool granny = klass == G::AI_Granny, grandpa = klass == G::AI_Grandpa;
    switch (action)
    {
    case 1: FreezeOne(e->ai, e->agent, g_FrozenOne); break;
    case 2: CallV(U::SetEnabled, e->ai, true); Unfreeze(g_FrozenOne, e->ai); break;
    case 3:
        if (granny) KnockOut(true);
        else if (grandpa) KnockOut(false);
        else if (void* go = GameObjectOf(e->ai)) { CallV(U::GoSetActive, go, false); g_Despawned.push_back(go); }
        break;
    case 4: if (granny) il2::Invoke(G::PepperGranny, e->ai, nullptr); if (grandpa) il2::Invoke(G::PepperGrandpa, e->ai, nullptr); break;
    case 5: if (granny) il2::Invoke(G::IceGranny, e->ai, nullptr); if (grandpa) il2::Invoke(G::IceGrandpa, e->ai, nullptr); break;
    case 6:  // Teleport to
        if (g_Cc)
        {
            Vec3 to = PosOf(e->ai), me = PosOf(g_Cc);
            Vec3 away = Sub(me, to);
            away.y = 0;
            Vec3 dest = Add(to, Mul(Len(away) > 0.1f ? Norm(away) : Vec3{ 0, 0, 1 }, 1.5f));
            dest.y += 0.1f;
            CallV(U::ColSetEnabled, g_Cc, false);
            CallV(U::SetPosition, TransformOf(g_Cc), dest);
            CallV(U::ColSetEnabled, g_Cc, !g_State.noclip);
            g_LastPos = dest;
        }
        break;
    case 7:  // Bring
        if (g_Cc)
        {
            Vec3 fwd = g_PlayerCam ? Call<Vec3>(U::GetForward, TransformOf(g_PlayerCam)) : Vec3{ 0, 0, 1 };
            fwd.y = 0;
            MoveEnemy(e->ai, e->agent, Add(PosOf(g_Cc), Mul(Norm(fwd), 2.5f)));
        }
        break;
    case 8:  // Fling
    {
        Vec3 from = PosOf(e->ai), me = g_Cc ? PosOf(g_Cc) : from;
        Vec3 dir = Sub(from, me);
        dir.y = 0;
        if (Len(dir) < 0.1f) dir = { 0, 0, 1 };
        MoveEnemy(e->ai, e->agent, Add(from, Mul(Norm(dir), 20.0f)));
        break;
    }
    case 9:
        if (void* go = GameObjectOf(e->ai)) { CallV(U::GoSetActive, go, false); g_Despawned.push_back(go); }
        break;
    }
}

// ============================================================================ ESP

static void WriteEsp()
{
    g_State.screenW = (float)Call<int>(U::ScreenW);
    g_State.screenH = (float)Call<int>(U::ScreenH);
    float screenH = g_State.screenH;
    void* cam = g_PlayerCam;
    if (!cam) { g_State.espCount = 0; return; }
    Vec3 camPos = PosOf(cam);

    int count = 0;
    for (auto& e : g_Enemies)
    {
        if (count >= MAX_ESP) break;
        if (!Alive(e.ai) || !ActiveGO(e.ai)) continue;

        Vec3 o = PosOf(e.ai);
        float h = 1.8f;
        if (e.head && Alive(e.head)) h = PosOf(e.head).y - o.y + 0.25f;
        else if (e.agent && Alive(e.agent)) { float ah = Call<float>(U::AgentHeight, e.agent); if (ah > 0.3f) h = ah; }
        h = std::clamp(h, 0.4f, 3.5f);
        float r = h * 0.22f;
        Vec3 mn{ o.x - r, o.y, o.z - r }, mx{ o.x + r, o.y + h, o.z + r };

        float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f, corners[16];
        bool allFront = true;
        for (int c = 0; c < 8; ++c)
        {
            Vec3 w{ (c & 1) ? mx.x : mn.x, (c & 2) ? mx.y : mn.y, (c & 4) ? mx.z : mn.z };
            Vec3 p = Call<Vec3>(U::CamW2S, cam, w);
            if (p.z <= 0.05f) { allFront = false; break; }
            float y = screenH - p.y;
            corners[c * 2] = p.x;
            corners[c * 2 + 1] = y;
            x0 = std::min(x0, p.x); x1 = std::max(x1, p.x);
            y0 = std::min(y0, y); y1 = std::max(y1, y);
        }
        bool onScreen = allFront;
        if (!allFront)
        {
            Vec3 feet = Call<Vec3>(U::CamW2S, cam, Vec3{ o.x, o.y, o.z });
            Vec3 head = Call<Vec3>(U::CamW2S, cam, Vec3{ o.x, o.y + h, o.z });
            if (feet.z > 0.05f && head.z > 0.05f)
            {
                float top = screenH - head.y, bottom = screenH - feet.y, hw = fabsf(bottom - top) * 0.22f;
                x0 = std::min(head.x, feet.x) - hw; x1 = std::max(head.x, feet.x) + hw;
                y0 = std::min(top, bottom); y1 = std::max(top, bottom);
                onScreen = true;
            }
        }

        EspEntry& out = g_State.esp[count];
        out.minX = x0; out.minY = y0; out.maxX = x1; out.maxY = y1;
        out.distance = Len(Sub(camPos, Vec3{ o.x, o.y + h / 2, o.z }));
        out.kind = e.kind;
        out.onScreen = onScreen ? 1 : 0;
        g_State.espId[count] = e.id;
        g_State.espCornersValid[count] = allFront ? 1 : 0;
        if (allFront) memcpy(g_State.espCorners[count], corners, sizeof(corners));
        WriteStr(g_State.espLabel[count], ESP_LABEL_LEN, e.label);
        ++count;
    }
    g_State.espCount = count;
}

// ============================================================================ frame

static int Take(int32_t& v) { int r = v; v = 0; return r; }

static void UpdateInfo()
{
    g_State.inGame = g_Fps ? 1 : 0;
    g_State.hasSavedPos = 0;
    g_State.currentDay = g_Days ? il2::Get<float>(g_Days, G::daysCurrentDay) : 0.0f;
    if (g_Cc)
    {
        Vec3 p = PosOf(g_Cc);
        g_State.posX = p.x; g_State.posY = p.y; g_State.posZ = p.z;
    }

    static size_t sentItems = (size_t)-1;
    if (sentItems != g_Items.size())
    {
        sentItems = g_Items.size();
        int n = std::min((int)g_Items.size(), MAX_ITEMS);
        for (int i = 0; i < n; ++i) WriteStr(g_State.items[i], ITEM_NAME_LEN, g_Items[i]);
        g_State.itemCount = n;
    }

    g_State.coins = g_CoinKey.empty() ? -1 : 1;
    g_State.coinValue = g_Coins;
    g_State.coinKeyType = g_CoinKey.empty() ? 0 : g_CoinFloat ? 2 : 1;
    g_State.coinKeyConfirmed = g_CoinConfirmed ? 1 : 0;
    WriteStr(g_State.coinKey, sizeof(g_State.coinKey), g_CoinKey);
    g_State.inMainMenu = g_MainMenu ? 1 : 0;
    g_State.inCapsuleShop = g_Capsule ? 1 : 0;
    g_State.explorerStatus = 0;

    char diag[DEBUG_LEN];
    std::string enemies;
    for (auto& e : g_Enemies) { if (!enemies.empty()) enemies += ", "; enemies += e.label; }
    snprintf(diag, sizeof(diag), "Standalone | Player: %s | Camera: %s | Enemies: %s | Items: %d",
             g_Fps ? "found" : "not found", g_PlayerCam ? "found" : "none",
             enemies.empty() ? "none" : enemies.c_str(), (int)g_Items.size());
    WriteStr(g_State.debug, DEBUG_LEN, diag);
}

static Vec3 g_SavedPos;
static bool g_HasSavedPos;

static void RunActions()
{
    if (Take(g_State.actKnockGranny)) KnockOut(true);
    if (Take(g_State.actKnockGrandpa)) KnockOut(false);
    if (Take(g_State.actKnockAll)) { KnockOut(true); KnockOut(false); }
    if (Take(g_State.actSavePos) && g_Cc) { g_SavedPos = PosOf(g_Cc); g_HasSavedPos = true; }
    if (Take(g_State.actLoadPos) && g_Cc && g_HasSavedPos)
    {
        CallV(U::ColSetEnabled, g_Cc, false);
        CallV(U::SetPosition, TransformOf(g_Cc), g_SavedPos);
        CallV(U::ColSetEnabled, g_Cc, !g_State.noclip);
        g_LastPos = g_SavedPos;
    }
    if (int d = Take(g_State.actDayMinus)) if (g_Days) il2::Set<float>(g_Days, G::daysCurrentDay, std::max(1.0f, il2::Get<float>(g_Days, G::daysCurrentDay) - d));
    if (int d = Take(g_State.actDayPlus)) if (g_Days) il2::Set<float>(g_Days, G::daysCurrentDay, il2::Get<float>(g_Days, G::daysCurrentDay) + d);
    if (int item = Take(g_State.actGiveItem))
        if (g_Inventory && item - 1 < (int)g_Items.size())
        {
            void* args[1] = { il2::Str(g_Items[item - 1].c_str()) };
            il2::Invoke(G::PickupItem, g_Inventory, args);
        }
    if (int c = Take(g_State.actAddCoins)) AddCoins((float)c);
    if (Take(g_State.actUnlockAll)) UnlockEverything();
    if (Take(g_State.actUnlockSkins)) UnlockAllColours();
    if (int a = Take(g_State.radialAction)) RunWheel(a, g_State.radialTarget);
    if (Take(g_State.actSaveConfig)) SaveSettings();
    g_State.actToggleExplorer = 0;
}

static void Tick()
{
    float now = Call<float>(U::UnscaledTime);
    if (now >= g_NextRefresh)
    {
        g_NextRefresh = now + 1.0f;
        RefreshScene();
        ApplyFreeze();
        RefreshShop(now);
    }

    if (g_State.menuOpen || g_State.radialOpen)
    {
        CallV(U::SetLockState, 0);
        CallV(U::SetVisible, true);
    }

    RunActions();
    ResolveCamera();
    ApplyNoclip();
    ApplyFullbright();
    ApplyTimeScale();
    ApplySpeed();
    ApplyOrbit();
    WriteEsp();
    g_State.hasSavedPos = g_HasSavedPos ? 1 : 0;
    UpdateInfo();
    g_State.hasSavedPos = g_HasSavedPos ? 1 : 0;
    FlushLogToState();
}

// ============================================================================ hooks

using VoidThis = void (*)(void*, const void*);
using PreCullFn = void (*)(void*, const void*);
using TriggerFn = void (*)(void*, void*, const void*);
using GetFloatFn = float (*)(void*, float, const void*);
using GetIntFn = int (*)(void*, int, const void*);

static PreCullFn o_FireOnPreCull;
static int g_LastFrame = -1;

static void h_FireOnPreCull(void* cam, const void* mi)
{
    int frame = Call<int>(U::FrameCount);
    if (frame != g_LastFrame)
    {
        g_LastFrame = frame;
        Tick();
    }
    ApplyFov(cam);
    o_FireOnPreCull(cam, mi);
}

// One detour per hooked "void Method()" so each keeps its own original pointer.
template <int N> struct VoidHook
{
    static inline VoidThis orig;
    static inline int mode;  // 0 = god mode block, 1 = invisible block, 2 = shop context
    static void Detour(void* self, const void* mi)
    {
        if (mode == 0 && g_State.godMode) return;
        if (mode == 1 && g_State.invisible) return;
        if (mode == 2) { ++g_ShopDepth; orig(self, mi); g_ShopDepth = std::max(0, g_ShopDepth - 1); return; }
        orig(self, mi);
    }
};

static bool IsPlayerCollider(void* col)
{
    if (!g_Cc || !Alive(col)) return false;
    void* target = TransformOf(g_Cc);
    for (void* t = TransformOf(col); t && Alive(t); t = Call<void*>(U::TGetParent, t))
        if (t == target) return true;
    return false;
}

template <int N> struct TriggerHook
{
    static inline TriggerFn orig;
    static void Detour(void* self, void* other, const void* mi)
    {
        if ((g_State.godMode || g_State.orbitEnabled) && IsPlayerCollider(other)) return;
        orig(self, other, mi);
    }
};

static GetFloatFn o_GetFloat;
static GetIntFn o_GetInt;
static float h_GetFloat(void* key, float def, const void* mi) { float v = o_GetFloat(key, def, mi); RecordPref(key, true); return v; }
static int h_GetInt(void* key, int def, const void* mi) { int v = o_GetInt(key, def, mi); RecordPref(key, false); return v; }

static bool Hook(void* target, void* detour, void** orig, const char* name)
{
    if (!target) { Game_Log("Hook %s: not found", name); return false; }
    if (MH_CreateHook(target, detour, orig) != MH_OK || MH_EnableHook(target) != MH_OK)
    {
        Game_Log("Hook %s: failed", name);
        return false;
    }
    return true;
}

template <int N> static void HookVoid(void* klass, const char* method, int mode)
{
    const void* mi = il2::Method(klass, method, 0);
    VoidHook<N>::mode = mode;
    Hook(mi ? *(void**)mi : nullptr, (void*)&VoidHook<N>::Detour, (void**)&VoidHook<N>::orig, method);
}

static void InstallHooks()
{
    MH_Initialize();
    Hook(U::FireOnPreCull.ptr, (void*)&h_FireOnPreCull, (void**)&o_FireOnPreCull, "Camera.FireOnPreCull");

    // God mode: the ways PlayerStatus kills or hurts the player.
    HookVoid<0>(G::PlayerStatus, "GrannyCaughtYou", 0);
    HookVoid<1>(G::PlayerStatus, "GrannyCaughtYouBed", 0);
    HookVoid<2>(G::PlayerStatus, "GrandpaCaughtYou", 0);
    HookVoid<3>(G::PlayerStatus, "GrandpaCaughtYouBed", 0);
    HookVoid<4>(G::PlayerStatus, "KnockDeath", 0);
    HookVoid<5>(G::PlayerStatus, "NormalDeath", 0);
    HookVoid<6>(G::PlayerStatus, "MomDeath", 0);
    HookVoid<7>(G::PlayerStatus, "DamageSpider", 0);
    HookVoid<8>(G::PlayerStatus, "DamageFeet", 0);
    // Invisible.
    HookVoid<9>(G::AI_Granny, "SensorPlayer", 1);
    HookVoid<10>(G::AI_Grandpa, "SensorPlayer", 1);
    // Shop context for coin detection.
    HookVoid<11>(G::MainMenu, "Update", 2);
    HookVoid<12>(G::ColorToyCapsule, "AttemptInsertCoin", 2);
    HookVoid<13>(G::ColorToyCapsule, "UpdateRarityUI", 2);
    HookVoid<14>(G::MainMenu, "AttemptBuy_NM", 2);
    HookVoid<15>(G::MainMenu, "AttemptBuy_GR", 2);
    HookVoid<16>(G::MainMenu, "AttemptBuy_EX", 2);

    const void* t1 = il2::Method(G::AI_Granny, "OnTriggerStay", 1);
    const void* t2 = il2::Method(G::AI_Grandpa, "OnTriggerStay", 1);
    Hook(t1 ? *(void**)t1 : nullptr, (void*)&TriggerHook<0>::Detour, (void**)&TriggerHook<0>::orig, "AI_Granny.OnTriggerStay");
    Hook(t2 ? *(void**)t2 : nullptr, (void*)&TriggerHook<1>::Detour, (void**)&TriggerHook<1>::orig, "AI_Grandpa.OnTriggerStay");

    Hook(U::PrefsGetFloat.ptr, (void*)&h_GetFloat, (void**)&o_GetFloat, "PlayerPrefs.GetFloat");
    Hook(U::PrefsGetInt.ptr, (void*)&h_GetInt, (void**)&o_GetInt, "PlayerPrefs.GetInt");
}

// ============================================================================ entry

bool Game_Start(const std::string& dir)
{
    g_LogPath = dir + "Cluado.log";
    g_IniPath = dir + "Cluado.ini";
    if (FILE* f = fopen(g_LogPath.c_str(), "w")) fclose(f);
    LoadSettings();

    for (int i = 0; i < 600 && !il2::Init(); ++i) Sleep(100);
    if (!il2::Init()) { Game_Log("GameAssembly.dll not found"); return false; }
    for (int i = 0; i < 1200 && !il2::Ready(); ++i) Sleep(100);
    if (!il2::Ready()) { Game_Log("IL2CPP never became ready"); return false; }
    // Attach only while resolving; everything else runs on Unity's main thread from the hooks.
    void* thread = il2::AttachThread();
    bool ok = Resolve();
    il2::DetachThread(thread);
    if (!ok)
        return false;
    InstallHooks();
    Game_Log("Cluado standalone loaded. Press Delete for the menu.");
    return true;
}
