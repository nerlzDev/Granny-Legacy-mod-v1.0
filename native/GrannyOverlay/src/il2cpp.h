#pragma once
// Minimal IL2CPP runtime access for the standalone build: classes, methods, fields and icalls
// looked up by name through GameAssembly.dll's exported il2cpp_* API.
#include <cstdint>
#include <string>

struct Vec3 { float x, y, z; };
struct Quat { float x, y, z, w; };
struct Color { float r, g, b, a; };
struct Rect { float x, y, w, h; };
struct Bounds { Vec3 center, extents; };
struct NavMeshHit { Vec3 position, normal; float distance; int32_t mask; int32_t hit; };

namespace il2
{
    // A callable method: the native code pointer plus its MethodInfo (passed as the hidden last argument).
    struct Fn
    {
        void* ptr = nullptr;
        const void* mi = nullptr;
        explicit operator bool() const { return ptr != nullptr; }
    };

    bool Init();                     // load the API (GameAssembly.dll must be loaded)
    bool Ready();                    // domain + Assembly-CSharp available
    void* AttachThread();
    void DetachThread(void* thread);

    void* Class(const char* image, const char* ns, const char* name);
    const void* Method(void* klass, const char* name, int argc);
    // Method by name; falls back to the icall (for members stripped from metadata).
    Fn Func(void* klass, const char* name, int argc, const char* icall = nullptr);
    int Field(void* klass, const char* name);  // instance field offset, -1 if missing
    void* TypeObject(void* klass);             // System.Type for FindObjectsOfType/AddComponent
    void* ClassOf(void* obj);
    const char* ClassName(void* klass);
    void* New(void* klass);
    void* Str(const char* utf8);
    std::string Utf8(void* str);

    // Safe call that catches managed exceptions. args: pointers to values, or object pointers.
    // Returns the boxed result (or the object for reference types); null on exception.
    void* Invoke(const void* method, void* obj, void** args, bool* failed = nullptr);
    void* Unbox(void* boxed);

    // Arrays: Il2CppArray header is 32 bytes on x64, then the elements.
    inline uintptr_t ArrayLength(void* arr) { return arr ? *(uintptr_t*)((char*)arr + 24) : 0; }
    inline void** ArrayData(void* arr) { return (void**)((char*)arr + 32); }

    bool Alive(void* unityObject);   // false for null or destroyed UnityEngine.Object

    template <typename T> inline T Get(void* obj, int off) { return off >= 0 && obj ? *(T*)((char*)obj + off) : T{}; }
    template <typename T> inline void Set(void* obj, int off, T v) { if (off >= 0 && obj) *(T*)((char*)obj + off) = v; }

    // Direct call: Call<Ret>(fn, this-or-first-arg, args...). Structs > 8 bytes go by pointer (MS x64 ABI),
    // which GCC on Windows does too, so value types can be passed and returned as declared.
    template <typename R, typename... A> inline R Call(const Fn& f, A... a)
    {
        if (!f.ptr) return R{};
        return ((R(*)(A..., const void*))f.ptr)(a..., f.mi);
    }
    template <typename... A> inline void CallV(const Fn& f, A... a)
    {
        if (f.ptr) ((void (*)(A..., const void*))f.ptr)(a..., f.mi);
    }
}
