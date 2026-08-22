#pragma once

#include "FrameBufferTransport.hpp"

#include <Obscura/IEngine.hpp>
#include <Obscura/IRHI.hpp>
#include <Obscura/Module.hpp>
#include <Obscura/Types.hpp>

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QElapsedTimer>
#include <QtCore/QString>
#include <QtQml/qqmlregistration.h>

#include <filesystem>
#include <memory>

namespace ObscuraEditor
{
    class EditorApplication : public QObject
    {
        Q_OBJECT
        QML_ELEMENT
        QML_SINGLETON

        Q_PROPERTY(QString engineVersion READ getEngineVersion NOTIFY engineInitialized)
        Q_PROPERTY(QString rhiName READ getRhiName NOTIFY engineInitialized)
        Q_PROPERTY(double fps READ getFps NOTIFY statsUpdated)
        Q_PROPERTY(quint64 frameCount READ getFrameCount NOTIFY statsUpdated)
        Q_PROPERTY(bool isEngineRunning READ isEngineRunning NOTIFY engineInitialized)
        Q_PROPERTY(bool vsyncEnabled READ isVsyncEnabled WRITE setVsyncEnabled NOTIFY vsyncChanged)

    public:
        explicit EditorApplication(QObject *parent = nullptr);
        ~EditorApplication() override;

        Q_INVOKABLE bool initialize(const QString &engineDllPath = QString());
        Q_INVOKABLE void shutdown();
        Q_INVOKABLE void requestExit();

        void onViewportResized(std::uint32_t width, std::uint32_t height);
        void handleMouseMove(float x, float y);
        void handleMouseButton(int button, bool pressed, float x, float y);
        void handleKey(int key, bool pressed);
        void setTickingEnabled(bool enabled);
        void setVsyncEnabled(bool enabled);

        [[nodiscard]] FrameBufferTransport& getTransport() noexcept { return m_Transport; }
        [[nodiscard]] QString getEngineVersion() const { return m_EngineVersion; }
        [[nodiscard]] QString getRhiName() const { return m_RhiName; }
        [[nodiscard]] double getFps() const { return m_CurrentFps; }
        [[nodiscard]] quint64 getFrameCount() const { return m_FrameCount; }
        [[nodiscard]] bool isEngineRunning() const { return m_Engine != nullptr; }
        [[nodiscard]] bool isVsyncEnabled() const noexcept { return m_VsyncEnabled; }
        [[nodiscard]] Obscura::IRHI* getRHI() const noexcept { return m_Engine ? m_Engine->GetRHI() : nullptr; }

    signals:
        void frameRendered();
        void engineInitialized(bool success);
        void statsUpdated();
        void vsyncChanged();
        void logMessage(const QString &message);

    private slots:
        void onTick();

    private:
        Obscura::Module        m_EngineModule;
        Obscura::IEngine*      m_Engine = nullptr;
        Obscura::DestroyEngineFn m_DestroyEngineFn = nullptr;

        FrameBufferTransport   m_Transport;
        QTimer                 m_TickTimer;
        QElapsedTimer          m_DeltaTimer;
        QElapsedTimer          m_FpsTimer;

        QString                m_EngineVersion = "N/A";
        QString                m_RhiName       = "N/A";
        double                 m_CurrentFps    = 0.0;
        quint64                m_FrameCount    = 0;
        quint64                m_FpsFrameCount = 0;

        std::uint32_t          m_ViewportWidth  = 1280;
        std::uint32_t          m_ViewportHeight = 720;
        bool                   m_TickingEnabled = true;
        bool                   m_VsyncEnabled   = false;
    };
}

