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
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtCore/QList>
#include <QtCore/QMap>
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
        Q_PROPERTY(double frameTime READ getFrameTime NOTIFY statsUpdated)
        Q_PROPERTY(quint64 frameCount READ getFrameCount NOTIFY statsUpdated)
        Q_PROPERTY(bool isEngineRunning READ isEngineRunning NOTIFY engineInitialized)
        Q_PROPERTY(bool vsyncEnabled READ isVsyncEnabled WRITE setVsyncEnabled NOTIFY vsyncChanged)
        Q_PROPERTY(QString currentScenePath READ getCurrentScenePath NOTIFY scenePathChanged)

    public:
        explicit EditorApplication(QObject *parent = nullptr);
        ~EditorApplication() override;

        Q_INVOKABLE bool initialize(const QString &engineDllPath = QString());
        Q_INVOKABLE void shutdown();
        Q_INVOKABLE void requestExit();

        // Scene Serialization & Management
        Q_INVOKABLE bool newScene();
        Q_INVOKABLE bool saveScene(const QString &filePath = QString());
        Q_INVOKABLE bool saveSceneAs(const QString &filePath);
        Q_INVOKABLE bool loadScene(const QString &filePath);
        [[nodiscard]] QString getCurrentScenePath() const { return m_CurrentScenePath; }

        // Entity ECS manipulation
        Q_INVOKABLE QVariantList getEntityList();
        Q_INVOKABLE QVariant     getEntity(const QString &uuidHex);
        Q_INVOKABLE QString      createEntity(const QString &name = "Entity", const QString &parentUuidHex = QString());
        Q_INVOKABLE bool         destroyEntity(const QString &uuidHex);
        Q_INVOKABLE bool         setEntityName(const QString &uuidHex, const QString &name);
        Q_INVOKABLE bool         setEntityTransform(const QString &uuidHex, double px, double py, double pz, double rx, double ry, double rz, double sx, double sy, double sz);
        Q_INVOKABLE bool         setEntitySprite2D(const QString &uuidHex, double cr, double cg, double cb, double ca, double uvOx, double uvOy, double uvSx, double uvSy, bool visible);
        Q_INVOKABLE bool         addEntityComponent(const QString &uuidHex, const QString &componentType);
        Q_INVOKABLE bool         removeEntityComponent(const QString &uuidHex, const QString &componentType);
        Q_INVOKABLE bool         setEntityTexture(const QString &uuidHex, const QString &filePath);
        Q_INVOKABLE QStringList  getAvailableTextures();
        Q_INVOKABLE QString      getTexturePathForSlot(int slot);
        Q_INVOKABLE void         clearLogs();

        void onViewportAspectResized(std::uint32_t width, std::uint32_t height);
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
        [[nodiscard]] double getFrameTime() const { return m_FrameTime; }
        [[nodiscard]] quint64 getFrameCount() const { return m_FrameCount; }
        [[nodiscard]] bool isEngineRunning() const { return m_Engine != nullptr; }
        [[nodiscard]] bool isVsyncEnabled() const noexcept { return m_VsyncEnabled; }
        [[nodiscard]] Obscura::IRHI* getRHI() const noexcept { return m_Engine ? m_Engine->GetRHI() : nullptr; }

    signals:
        void frameRendered();
        void engineInitialized(bool success);
        void statsUpdated();
        void vsyncChanged();
        void scenePathChanged();
        void logMessage(const QString &message, int level = 2);
        void sceneEntitiesChanged();
        void entityUpdated(const QString &uuidHex);

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
        QString                m_CurrentScenePath = "";
        double                 m_CurrentFps    = 0.0;
        double                 m_FrameTime     = 0.0;
        quint64                m_FrameCount    = 0;
        quint64                m_FpsFrameCount = 0;

        std::uint32_t          m_ViewportWidth  = 1280;
        std::uint32_t          m_ViewportHeight = 720;
        bool                   m_TickingEnabled = true;
        bool                   m_VsyncEnabled   = false;

        bool                   m_RightMouseDown  = false;
        bool                   m_MiddleMouseDown = false;
        bool                   m_LeftMouseDown   = false;
    };

}

