#pragma once

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#endif

#include <array>
#include <filesystem>
#include <string_view>
#include <vector>

namespace Obscura::VFS
{
#ifdef _WIN32
    inline std::filesystem::path GetModuleFullName()
    {
        constexpr DWORD kMaxPath = 2048;
        std::array<wchar_t, kMaxPath> buffer = {};
        DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        return std::filesystem::path(std::wstring_view(buffer.data(), length));
    }

    inline std::filesystem::path GetModuleDirectory()
    {
        return GetModuleFullName().parent_path();
    }
#elif defined(__linux__) || defined(__GNUC__)
#error "Unsupported Platform"
#endif

    // Resolves a relative asset/resource path relative to the executable location,
    // working directory, parent directories, or repository source root.
    inline std::filesystem::path ResolvePath(const std::filesystem::path& relativePath)
    {
        if (relativePath.is_absolute() && std::filesystem::exists(relativePath))
        {
            return std::filesystem::weakly_canonical(relativePath);
        }

        // Build list of relative path variants (with and without "Resources/", "Obscura/Resources/")
        std::vector<std::filesystem::path> pathVariants;
        pathVariants.push_back(relativePath);

        std::string relStr = relativePath.generic_string();
        if (relStr.rfind("Resources/", 0) == 0)
        {
            pathVariants.push_back(relStr.substr(10)); // stripped "Resources/"
        }
        if (relStr.rfind("Obscura/Resources/", 0) == 0)
        {
            pathVariants.push_back(relStr.substr(18)); // stripped "Obscura/Resources/"
        }

        // Roots to search: executable directory and working directory + up to 6 parent levels
        std::vector<std::filesystem::path> rootDirs;
        auto AddRootAndAncestors = [&](std::filesystem::path base)
        {
            for (int i = 0; i < 6 && !base.empty(); ++i)
            {
                rootDirs.push_back(base);
                auto parent = base.parent_path();
                if (parent == base) break;
                base = parent;
            }
        };

        AddRootAndAncestors(GetModuleDirectory());
        AddRootAndAncestors(std::filesystem::current_path());

        // Subdirectories to probe within each root
        const std::vector<std::filesystem::path> subDirs = {
            "",
            "Resources",
            "Resources/Shaders",
            "Obscura/Resources",
            "Obscura/Resources/Shaders",
            "Shaders",
            "bin/Debug/Resources",
            "bin/Release/Resources",
            "Build/bin/Debug/Resources"
        };

        for (const auto& variant : pathVariants)
        {
            for (const auto& root : rootDirs)
            {
                for (const auto& sub : subDirs)
                {
                    auto candidate = root / sub / variant;
                    std::error_code ec;
                    if (std::filesystem::exists(candidate, ec) && !std::filesystem::is_directory(candidate, ec))
                    {
                        return std::filesystem::weakly_canonical(candidate);
                    }
                }
            }
        }

        return {};
    }
}
