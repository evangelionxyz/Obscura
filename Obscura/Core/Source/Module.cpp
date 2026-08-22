#include "Obscura/Module.hpp"
#include "Obscura/Logger.hpp"

#include <string>

#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <Windows.h>
#else
    #include <dlfcn.h>
#endif

namespace Obscura
{
    Module::~Module()
    {
        Unload();
    }

    Module::Module(Module&& other) noexcept
        : m_Handle(other.m_Handle)
    {
        other.m_Handle = nullptr;
    }

    Module& Module::operator=(Module&& other) noexcept
    {
        if (this != &other)
        {
            Unload();
            m_Handle = other.m_Handle;
            other.m_Handle = nullptr;
        }
        return *this;
    }

    bool Module::Load(const std::filesystem::path& filepath)
    {
        Unload();

    #if defined(_WIN32)
        std::wstring pathStr = filepath.wstring();
        m_Handle = static_cast<void*>(LoadLibraryW(pathStr.c_str()));
        if (!m_Handle)
        {
            DWORD err = GetLastError();
            LOG_ERROR("[Module] LoadLibraryW failed for '{}' (error: {})",
                filepath.string(), err);
        }
    #else
        m_Handle = dlopen(filepath.string().c_str(), RTLD_NOW | RTLD_LOCAL);
        if (!m_Handle)
        {
            LOG_ERROR("[Module] dlopen failed for '{}': {}",
                filepath.string(), dlerror());
        }
    #endif

        return m_Handle != nullptr;
    }

    void Module::Unload()
    {
        if (m_Handle)
        {
#if defined(_WIN32)
            FreeLibrary(static_cast<HMODULE>(m_Handle));
#else
            dlclose(m_Handle);
#endif
            m_Handle = nullptr;
        }
    }
}
