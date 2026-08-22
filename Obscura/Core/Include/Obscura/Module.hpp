#pragma once

#include <filesystem>
#include <string>

// =============================================================================
// Obscura Engine — Dynamic Library Loader
// =============================================================================
//
// Cross-platform wrapper for loading shared libraries at runtime.
// Currently supports Windows (LoadLibrary/FreeLibrary).
//
// Usage:
//   Obscura::Module mod;
//   if (mod.Load("MyLib.dll")) {
//       auto fn = mod.GetSymbol<void(*)()>("MyFunction");
//       if (fn) fn();
//   }
// =============================================================================

namespace Obscura
{

    class Module
    {
    public:
        Module() = default;
        ~Module();

        // Non-copyable
        Module(const Module&) = delete;
        Module& operator=(const Module&) = delete;

        // Movable
        Module(Module&& other) noexcept;
        Module& operator=(Module&& other) noexcept;

        /// Load a shared library from the given file path.
        /// Returns true on success.
        bool Load(const std::filesystem::path& filepath);

        /// Unload the currently loaded library.
        void Unload();

        /// Returns true if a library is currently loaded.
        [[nodiscard]] bool IsLoaded() const noexcept { return m_Handle != nullptr; }

        /// Look up a symbol (function pointer) by name.
        /// Returns nullptr if not found or if no library is loaded.
        template<typename TFunc>
        [[nodiscard]] TFunc GetSymbol(const char* name) const;

        /// Returns the platform-native handle (HMODULE on Windows).
        [[nodiscard]] void* GetNativeHandle() const noexcept { return m_Handle; }

    private:
        void* m_Handle = nullptr;
    };

} // namespace Obscura

// =============================================================================
// Template implementation (must be in header)
// =============================================================================

#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <Windows.h>
#else
    #include <dlfcn.h>
#endif

template<typename TFunc>
TFunc Obscura::Module::GetSymbol(const char* name) const
{
    if (!m_Handle)
        return nullptr;

#if defined(_WIN32)
    return reinterpret_cast<TFunc>(
        GetProcAddress(static_cast<HMODULE>(m_Handle), name)
    );
#else
    return reinterpret_cast<TFunc>(dlsym(m_Handle, name));
#endif
}
