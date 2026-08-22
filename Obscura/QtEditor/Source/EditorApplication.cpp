#include "EditorApplication.hpp"
#include "EngineViewport.hpp"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>

#include <iostream>
#include <print>

namespace ObscuraEditor
{
    EditorApplication::EditorApplication(QObject *parent)
        : QObject(parent)
    {
        EngineViewport::SetApplicationInstance(this);

        connect(&m_TickTimer, &QTimer::timeout, this, &EditorApplication::onTick);
    }

    EditorApplication::~EditorApplication()
    {
        shutdown();
    }

    bool EditorApplication::initialize(const QString &engineDllPath)
    {
        if (m_Engine)
        {
            return true;
        }

        std::filesystem::path resolvedPath;
        if (!engineDllPath.isEmpty() && QFileInfo::exists(engineDllPath))
        {
            resolvedPath = engineDllPath.toStdString();
        }
        else
        {
            // Auto-search standard build paths
            const QString appDir = QCoreApplication::applicationDirPath();
            const QStringList candidatePaths = {
                appDir + "/Obscura.Engine.dll",
                appDir + "/../x86_64/Debug/Obscura.Engine.dll",
                appDir + "/../../x86_64/Debug/Obscura.Engine.dll",
                appDir + "/../../../Build/x86_64/Debug/Obscura.Engine.dll",
                "D:/Dev/TestDLL/Build/x86_64/Debug/Obscura.Engine.dll"
            };

            for (const auto &candidate : candidatePaths)
            {
                if (QFileInfo::exists(candidate))
                {
                    resolvedPath = QDir::toNativeSeparators(candidate).toStdString();
                    break;
                }
            }
        }

        if (resolvedPath.empty())
        {
            const QString err = "Obscura.Engine.dll could not be found in application directory or build paths.";
            qWarning() << "[QtEditor]" << err;
            emit logMessage("[Error] " + err);
            emit engineInitialized(false);
            return false;
        }

        qInfo() << "[QtEditor] Loading Engine module from:" << QString::fromStdString(resolvedPath.string());
        emit logMessage("[Host] Loading Engine module from: " + QString::fromStdString(resolvedPath.string()));

        if (!m_EngineModule.Load(resolvedPath))
        {
            const QString err = "Failed to load dynamic library: " + QString::fromStdString(resolvedPath.string());
            qCritical() << "[QtEditor]" << err;
            emit logMessage("[Error] " + err);
            emit engineInitialized(false);
            return false;
        }

        auto createEngineFn = m_EngineModule.GetSymbol<Obscura::CreateEngineFn>("CreateEngine");
        m_DestroyEngineFn   = m_EngineModule.GetSymbol<Obscura::DestroyEngineFn>("DestroyEngine");

        if (!createEngineFn)
        {
            const QString err = "Could not find 'CreateEngine' symbol in Engine DLL.";
            qCritical() << "[QtEditor]" << err;
            emit logMessage("[Error] " + err);
            m_EngineModule.Unload();
            emit engineInitialized(false);
            return false;
        }

        m_Engine = createEngineFn(Obscura::ENGINE_ABI_VERSION);
        if (!m_Engine)
        {
            const QString err = "Engine ABI version mismatch! Expected version " + QString::number(Obscura::ENGINE_ABI_VERSION);
            qCritical() << "[QtEditor]" << err;
            emit logMessage("[Error] " + err);
            m_EngineModule.Unload();
            emit engineInitialized(false);
            return false;
        }

        Obscura::EngineInitParams params{
            .WindowWidth  = m_ViewportWidth,
            .WindowHeight = m_ViewportHeight,
            .AppTitle     = "Obscura Engine Qt Editor"
        };

        if (!m_Engine->Initialize(params))
        {
            const QString err = "Engine::Initialize failed.";
            qCritical() << "[QtEditor]" << err;
            emit logMessage("[Error] " + err);
            shutdown();
            emit engineInitialized(false);
            return false;
        }

        m_EngineVersion = QString::fromUtf8(m_Engine->GetVersion());
        if (auto* rhi = m_Engine->GetRHI())
        {
            m_RhiName = QString::fromUtf8(rhi->GetName());
            rhi->SetRenderMode(Obscura::RenderMode::Offscreen);
            rhi->CreateOffscreenTarget(m_ViewportWidth, m_ViewportHeight);
        }

        qInfo() << "[QtEditor] Engine initialized successfully. Version:" << m_EngineVersion << "RHI:" << m_RhiName;
        emit logMessage("[Engine] Version: " + m_EngineVersion + " | RHI: " + m_RhiName);

        m_DeltaTimer.start();
        m_FpsTimer.start();
        m_TickTimer.start(m_VsyncEnabled ? 16 : 0);

        emit engineInitialized(true);
        return true;
    }

    void EditorApplication::shutdown()
    {
        m_TickTimer.stop();

        if (m_Engine)
        {
            qInfo() << "[QtEditor] Shutting down Engine...";
            emit logMessage("[Host] Initiating engine shutdown...");

            if (auto* rhi = m_Engine->GetRHI())
            {
                rhi->DestroyOffscreenTarget();
            }

            m_Engine->Shutdown();

            if (m_DestroyEngineFn)
            {
                m_DestroyEngineFn(m_Engine);
            }
            else
            {
                m_Engine->Destroy();
            }
            m_Engine = nullptr;
            m_EngineModule.Unload();

            qInfo() << "[QtEditor] Engine shutdown complete.";
            emit logMessage("[Host] Engine shutdown complete.");
        }

        m_EngineVersion = "N/A";
        m_RhiName = "N/A";
    }

    void EditorApplication::requestExit()
    {
        QCoreApplication::quit();
    }

    void EditorApplication::onViewportResized(std::uint32_t width, std::uint32_t height)
    {
        if (width == 0 || height == 0)
        {
            return;
        }

        m_ViewportWidth  = width;
        m_ViewportHeight = height;

        if (m_Engine)
        {
            if (auto* rhi = m_Engine->GetRHI())
            {
                if (rhi->ResizeOffscreenTarget(width, height))
                {
                    m_Transport.UpdateFromRHI(rhi);
                    emit frameRendered();
                }
            }
        }
    }

    void EditorApplication::handleMouseMove(float x, float y)
    {
        // Forward mouse move to engine input system
    }

    void EditorApplication::handleMouseButton(int button, bool pressed, float x, float y)
    {
        // Forward mouse button to engine input system
    }

    void EditorApplication::handleKey(int key, bool pressed)
    {
        // Forward key event to engine input system
    }

    void EditorApplication::setTickingEnabled(bool enabled)
    {
        m_TickingEnabled = enabled;
        if (enabled && !m_TickTimer.isActive())
        {
            m_DeltaTimer.restart();
            m_TickTimer.start(m_VsyncEnabled ? 16 : 0);
        }
        else if (!enabled && m_TickTimer.isActive())
        {
            m_TickTimer.stop();
        }
    }

    void EditorApplication::setVsyncEnabled(bool enabled)
    {
        if (m_VsyncEnabled != enabled)
        {
            m_VsyncEnabled = enabled;
            if (m_TickTimer.isActive())
            {
                m_TickTimer.setInterval(m_VsyncEnabled ? 16 : 0);
            }
            emit vsyncChanged();
        }
    }

    void EditorApplication::onTick()
    {
        if (!m_Engine || !m_TickingEnabled)
        {
            return;
        }

        const qint64 elapsedNs = m_DeltaTimer.nsecsElapsed();
        m_DeltaTimer.restart();
        const float deltaTime = static_cast<float>(elapsedNs) / 1'000'000'000.0f;

        m_Engine->Tick(deltaTime);
        m_FrameCount++;
        m_FpsFrameCount++;

        // Readback offscreen frame buffer to transport
        if (auto* rhi = m_Engine->GetRHI())
        {
            if (m_Transport.UpdateFromRHI(rhi))
            {
                emit frameRendered();
            }
        }

        // Update FPS calculation every 500 ms
        if (m_FpsTimer.elapsed() >= 500)
        {
            m_CurrentFps = static_cast<double>(m_FpsFrameCount) * 1000.0 / static_cast<double>(m_FpsTimer.elapsed());
            m_FpsFrameCount = 0;
            m_FpsTimer.restart();
            emit statsUpdated();
        }
    }
}
