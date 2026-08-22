#pragma once

#ifdef _WIN32
#include <Windows.h>
#endif

#include <array>
#include <filesystem>

namespace Obscura::VFS
{
#ifdef _WIN32
    inline std::filesystem::path GetModuleFullName()
    {
        constexpr DWORD kMaxPath = 260;
        std::array<wchar_t, kMaxPath> buffer = {};
        DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        return std::filesystem::path(buffer.data(), buffer.data() + length);
    }

    inline std::filesystem::path GetModuleDirectory()
    {
        return GetModuleFullName().parent_path();
    }
#elif defined(__linux__) || defined(__GNUC__)
#error "Unsupported Platform"
#endif
}
