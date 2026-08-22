#pragma once

#if defined(_WIN32)
    #define OBSCURA_DLL_EXPORT __declspec(dllexport)
    #define OBSCURA_DLL_IMPORT __declspec(dllimport)
#elif defined(__GNUC__) || defined(__clang__)
    #define OBSCURA_DLL_EXPORT __attribute__((visibility("default")))
    #define OBSCURA_DLL_IMPORT
#else
    #define OBSCURA_DLL_EXPORT
    #define OBSCURA_DLL_IMPORT
#endif

// --- Engine ---
#if defined(OBSCURA_ENGINE_EXPORTS)
    #define OBSCURA_ENGINE_API OBSCURA_DLL_EXPORT
#else
    #define OBSCURA_ENGINE_API OBSCURA_DLL_IMPORT
#endif

// --- Renderer ---
#if defined(OBSCURA_RENDERER_EXPORTS)
    #define OBSCURA_RENDERER_API OBSCURA_DLL_EXPORT
#else
    #define OBSCURA_RENDERER_API OBSCURA_DLL_IMPORT
#endif

// --- Scripting ---
#if defined(OBSCURA_SCRIPTING_EXPORTS)
    #define OBSCURA_SCRIPTING_API OBSCURA_DLL_EXPORT
#else
    #define OBSCURA_SCRIPTING_API OBSCURA_DLL_IMPORT
#endif

// --- Plugin ---
#if defined(OBSCURA_PLUGIN_EXPORTS)
    #define OBSCURA_PLUGIN_API OBSCURA_DLL_EXPORT
#else
    #define OBSCURA_PLUGIN_API OBSCURA_DLL_IMPORT
#endif
