#include "il2cpp.h"
#include <windows.h>
#include <cstring>

namespace
{
    HMODULE g_Module;

    void* (*domain_get)();
    void** (*domain_get_assemblies)(void* domain, size_t* size);
    void* (*assembly_get_image)(void* assembly);
    const char* (*image_get_name)(void* image);
    void* (*class_from_name)(void* image, const char* ns, const char* name);
    const void* (*class_get_method_from_name)(void* klass, const char* name, int argc);
    void* (*class_get_field_from_name)(void* klass, const char* name);
    size_t (*field_get_offset)(void* field);
    void* (*class_get_type)(void* klass);
    void* (*type_get_object)(void* type);
    void* (*string_new)(const char* str);
    void* (*resolve_icall)(const char* name);
    void* (*thread_attach)(void* domain);
    void (*thread_detach)(void* thread);
    void* (*object_get_class)(void* obj);
    const char* (*class_get_name)(void* klass);
    void* (*object_new)(void* klass);
    void* (*runtime_invoke)(const void* method, void* obj, void** params, void** exc);
    void* (*object_unbox)(void* obj);
    void* (*class_get_parent)(void* klass);

    int g_CachedPtrOffset = -1;

    template <typename T> bool Load(T& fn, const char* name)
    {
        fn = (T)GetProcAddress(g_Module, name);
        return fn != nullptr;
    }

    void* FindImage(const char* name)
    {
        void* domain = domain_get ? domain_get() : nullptr;
        if (!domain)
            return nullptr;
        size_t count = 0;
        void** assemblies = domain_get_assemblies(domain, &count);
        for (size_t i = 0; i < count; ++i)
        {
            void* image = assembly_get_image(assemblies[i]);
            if (image && (!name || strcmp(image_get_name(image), name) == 0))
                return image;
        }
        return nullptr;
    }
}

namespace il2
{
    bool Init()
    {
        g_Module = GetModuleHandleW(L"GameAssembly.dll");
        if (!g_Module)
            return false;
        bool ok = true;
        ok &= Load(domain_get, "il2cpp_domain_get");
        ok &= Load(domain_get_assemblies, "il2cpp_domain_get_assemblies");
        ok &= Load(assembly_get_image, "il2cpp_assembly_get_image");
        ok &= Load(image_get_name, "il2cpp_image_get_name");
        ok &= Load(class_from_name, "il2cpp_class_from_name");
        ok &= Load(class_get_method_from_name, "il2cpp_class_get_method_from_name");
        ok &= Load(class_get_field_from_name, "il2cpp_class_get_field_from_name");
        ok &= Load(field_get_offset, "il2cpp_field_get_offset");
        ok &= Load(class_get_type, "il2cpp_class_get_type");
        ok &= Load(type_get_object, "il2cpp_type_get_object");
        ok &= Load(string_new, "il2cpp_string_new");
        ok &= Load(resolve_icall, "il2cpp_resolve_icall");
        ok &= Load(thread_attach, "il2cpp_thread_attach");
        Load(thread_detach, "il2cpp_thread_detach");
        ok &= Load(object_get_class, "il2cpp_object_get_class");
        ok &= Load(class_get_name, "il2cpp_class_get_name");
        ok &= Load(object_new, "il2cpp_object_new");
        ok &= Load(runtime_invoke, "il2cpp_runtime_invoke");
        ok &= Load(object_unbox, "il2cpp_object_unbox");
        ok &= Load(class_get_parent, "il2cpp_class_get_parent");
        return ok;
    }

    bool Ready()
    {
        return domain_get && domain_get() && FindImage("Assembly-CSharp.dll");
    }

    void* AttachThread()
    {
        return domain_get && thread_attach ? thread_attach(domain_get()) : nullptr;
    }

    void DetachThread(void* thread)
    {
        if (thread && thread_detach)
            thread_detach(thread);
    }

    void* Class(const char* image, const char* ns, const char* name)
    {
        void* img = FindImage(image);
        return img ? class_from_name(img, ns, name) : nullptr;
    }

    const void* Method(void* klass, const char* name, int argc)
    {
        // Walk up the hierarchy so inherited members (Behaviour.enabled on an AI class) resolve too.
        for (void* k = klass; k; k = class_get_parent(k))
            if (const void* m = class_get_method_from_name(k, name, argc))
                return m;
        return nullptr;
    }

    Fn Func(void* klass, const char* name, int argc, const char* icall)
    {
        Fn f;
        if (klass)
            f.mi = Method(klass, name, argc);
        if (f.mi)
            f.ptr = *(void**)f.mi;  // MethodInfo::methodPointer is the first field
        else if (icall)
        {
            f.ptr = resolve_icall(icall);
            f.mi = nullptr;
        }
        return f;
    }

    int Field(void* klass, const char* name)
    {
        for (void* k = klass; k; k = class_get_parent(k))
            if (void* f = class_get_field_from_name(k, name))
                return (int)field_get_offset(f);
        return -1;
    }

    void* TypeObject(void* klass) { return klass ? type_get_object(class_get_type(klass)) : nullptr; }
    void* ClassOf(void* obj) { return obj ? object_get_class(obj) : nullptr; }
    const char* ClassName(void* klass) { return klass ? class_get_name(klass) : ""; }
    void* New(void* klass) { return klass ? object_new(klass) : nullptr; }
    void* Str(const char* utf8) { return string_new(utf8); }

    std::string Utf8(void* str)
    {
        if (!str)
            return {};
        int len = *(int32_t*)((char*)str + 16);
        const wchar_t* chars = (const wchar_t*)((char*)str + 20);
        if (len <= 0)
            return {};
        int size = WideCharToMultiByte(CP_UTF8, 0, chars, len, nullptr, 0, nullptr, nullptr);
        std::string out(size, '\0');
        WideCharToMultiByte(CP_UTF8, 0, chars, len, out.data(), size, nullptr, nullptr);
        return out;
    }

    void* Invoke(const void* method, void* obj, void** args, bool* failed)
    {
        if (failed) *failed = true;
        if (!method)
            return nullptr;
        void* exc = nullptr;
        void* result = runtime_invoke(method, obj, args, &exc);
        if (exc)
            return nullptr;
        if (failed) *failed = false;
        return result;
    }

    void* Unbox(void* boxed) { return boxed ? object_unbox(boxed) : nullptr; }

    bool Alive(void* obj)
    {
        if (!obj)
            return false;
        if (g_CachedPtrOffset < 0)
        {
            void* objectClass = Class("UnityEngine.CoreModule.dll", "UnityEngine", "Object");
            g_CachedPtrOffset = Field(objectClass, "m_CachedPtr");
            if (g_CachedPtrOffset < 0)
                g_CachedPtrOffset = 16;
        }
        return *(void**)((char*)obj + g_CachedPtrOffset) != nullptr;
    }
}
