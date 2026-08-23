#pragma once

#include <Obscura/IEngine.hpp>
#include <Obscura/Module.hpp>

#include <utility>

namespace Obscura
{
    class EngineModule
    {
    public:
        EngineModule() = default;

        ~EngineModule()
        {
            Shutdown();
        }

        EngineModule(const EngineModule &) = delete;
        EngineModule &operator=(const EngineModule &) = delete;

        EngineModule(EngineModule &&other) noexcept
            : m_Module(std::move(other.m_Module)), m_Engine(other.m_Engine)
        {
            other.m_Engine = nullptr;
        }

        EngineModule &operator=(EngineModule &&other) noexcept
        {
            if (this != &other)
            {
                Shutdown();
                m_Module = std::move(other.m_Module);
                m_Engine = other.m_Engine;
                other.m_Engine = nullptr;
            }
            return *this;
        }

        bool Load(const std::filesystem::path &path);
        void Shutdown();

        [[nodiscard]] bool IsLoaded() const noexcept { return m_Engine != nullptr; }
        [[nodiscard]] Obscura::IEngine *Get() const noexcept { return m_Engine; }
        [[nodiscard]] Obscura::IEngine *operator->() const noexcept { return m_Engine; }
        explicit operator bool() const noexcept { return m_Engine != nullptr; }

    private:
        Obscura::Module m_Module;
        Obscura::IEngine *m_Engine = nullptr;
    };
}
