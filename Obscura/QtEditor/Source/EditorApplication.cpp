#include "EditorApplication.hpp"
#include "EngineViewport.hpp"
#include <Obscura/Logger.hpp>

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

        Obscura::Logger::Init();
        Obscura::Logger::SetCallback([this](spdlog::level::level_enum level, const std::string &msg) {
            QString qMsg = QString::fromStdString(msg);
            int lvl = static_cast<int>(level);
            QMetaObject::invokeMethod(this, [this, lvl, qMsg]() {
                emit logMessage(qMsg, lvl);
            }, Qt::QueuedConnection);
        });

        connect(&m_TickTimer, &QTimer::timeout, this, &EditorApplication::onTick);
    }

    EditorApplication::~EditorApplication()
    {
        Obscura::Logger::ClearCallback();
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

    void EditorApplication::onViewportAspectResized(std::uint32_t width, std::uint32_t height)
    {
        if (width == 0 || height == 0)
        {
            return;
        }

        m_ViewportWidth  = width;
        m_ViewportHeight = height;

        if (m_Engine)
        {
            m_Engine->UpdateCameraProjection(width, height);
            onTick();
        }
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
            // 1. Resize RHI Offscreen Target first (creates new VkImages & VkImageViews with new width, height)
            if (auto* rhi = m_Engine->GetRHI())
            {
                rhi->ResizeOffscreenTarget(width, height);
            }

            // 2. Recreate framebuffers matching the new VkImageViews and update camera
            m_Engine->HandleViewportResize(width, height);

            onTick();
        }
    }

    void EditorApplication::handleMouseMove(float x, float y)
    {
        if (m_Engine)
        {
            m_Engine->HandleMouseMove(x, y, m_RightMouseDown, m_MiddleMouseDown, m_LeftMouseDown);
        }
    }

    void EditorApplication::handleMouseButton(int button, bool pressed, float x, float y)
    {
        // Qt::RightButton = 2, Qt::LeftButton = 1, Qt::MiddleButton = 4
        if (button == static_cast<int>(Qt::RightButton) || button == 2)
        {
            m_RightMouseDown = pressed;
        }
        else if (button == static_cast<int>(Qt::MiddleButton) || button == 4)
        {
            m_MiddleMouseDown = pressed;
        }
        else if (button == static_cast<int>(Qt::LeftButton) || button == 1)
        {
            m_LeftMouseDown = pressed;
        }

        if (m_Engine)
        {
            m_Engine->HandleMouseButton(button, pressed, x, y);
        }
    }

    void EditorApplication::handleKey(int key, bool pressed)
    {
        if (m_Engine)
        {
            m_Engine->HandleKey(key, pressed);
        }
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
            m_FrameTime = 1.0 / m_CurrentFps;

            m_FpsFrameCount = 0;
            m_FpsTimer.restart();
            emit statsUpdated();
        }
    }

    static QString formatUUID(std::uint64_t uuid)
    {
        return QString("0x%1").arg(uuid, 16, 16, QLatin1Char('0')).toUpper();
    }

    static std::uint64_t parseUUID(const QString &uuidStr)
    {
        QString clean = uuidStr.trimmed();
        if (clean.startsWith("0x", Qt::CaseInsensitive))
        {
            clean = clean.mid(2);
        }
        bool ok = false;
        std::uint64_t val = clean.toULongLong(&ok, 16);
        return ok ? val : 0;
    }

    QVariantList EditorApplication::getEntityList()
    {
        QVariantList list;
        if (!m_Engine)
        {
            return list;
        }

        std::uint32_t count = m_Engine->GetEntityCount();
        for (std::uint32_t i = 0; i < count; ++i)
        {
            Obscura::EntityDesc desc{};
            if (m_Engine->GetEntityDescByIndex(i, &desc))
            {
                QVariantMap item;
                item["uuid"]        = formatUUID(desc.uuid);
                item["name"]        = QString::fromUtf8(desc.name);
                item["parentUuid"]  = formatUUID(desc.parentUuid);
                item["type"]        = desc.hasSprite2D ? "Sprite2D" : "Entity";
                item["hasTransform"] = desc.hasTransform;
                item["hasSprite2D"]  = desc.hasSprite2D;
                list.append(item);
            }
        }
        return list;
    }

    QVariant EditorApplication::getEntity(const QString &uuidHex)
    {
        if (!m_Engine)
        {
            return QVariant();
        }

        std::uint64_t uuid = parseUUID(uuidHex);
        Obscura::EntityDesc desc{};
        if (!m_Engine->GetEntityDescByUUID(uuid, &desc))
        {
            return QVariant();
        }

        QVariantMap map;

        map["uuid"]        = formatUUID(desc.uuid);
        map["name"]        = QString::fromUtf8(desc.name);
        map["parentUuid"]  = formatUUID(desc.parentUuid);
        map["hasTransform"] = desc.hasTransform;
        map["posX"]        = desc.position[0];
        map["posY"]        = desc.position[1];
        map["posZ"]        = desc.position[2];
        map["rotX"]        = desc.rotation[0];
        map["rotY"]        = desc.rotation[1];
        map["rotZ"]        = desc.rotation[2];
        map["scaleX"]      = desc.scale[0];
        map["scaleY"]      = desc.scale[1];
        map["scaleZ"]      = desc.scale[2];

        map["hasSprite2D"]   = desc.hasSprite2D;
        map["colorR"]        = desc.spriteColor[0];
        map["colorG"]        = desc.spriteColor[1];
        map["colorB"]        = desc.spriteColor[2];
        map["colorA"]        = desc.spriteColor[3];
        map["textureHandle"] = formatUUID(desc.textureHandle);
        map["texturePath"]   = QString::fromUtf8(desc.texturePath);
        map["textureState"]  = static_cast<int>(desc.textureState);
        map["uvOffsetX"]     = desc.uvOffset[0];
        map["uvOffsetY"]     = desc.uvOffset[1];
        map["uvScaleX"]      = desc.uvScale[0];
        map["uvScaleY"]      = desc.uvScale[1];
        map["spriteVisible"] = desc.spriteVisible;

        return map;
    }

    QString EditorApplication::createEntity(const QString &name, const QString &parentUuidHex)
    {
        if (!m_Engine)
        {
            return QString();
        }

        std::uint64_t parentUuid = parseUUID(parentUuidHex);
        std::uint64_t newUuid = m_Engine->CreateEntity(name.toUtf8().constData(), parentUuid);
        QString uuidStr = formatUUID(newUuid);
        emit sceneEntitiesChanged();
        return uuidStr;
    }

    bool EditorApplication::destroyEntity(const QString &uuidHex)
    {
        if (!m_Engine)
        {
            return false;
        }

        std::uint64_t uuid = parseUUID(uuidHex);
        bool success = m_Engine->DestroyEntity(uuid);
        if (success)
        {
            emit sceneEntitiesChanged();
        }
        return success;
    }

    bool EditorApplication::setEntityName(const QString &uuidHex, const QString &name)
    {
        if (!m_Engine)
        {
            return false;
        }

        std::uint64_t uuid = parseUUID(uuidHex);
        bool success = m_Engine->SetEntityName(uuid, name.toUtf8().constData());
        if (success)
        {
            emit sceneEntitiesChanged();
            emit entityUpdated(uuidHex);
        }
        return success;
    }

    bool EditorApplication::setEntityTransform(const QString &uuidHex, double px, double py, double pz, double rx, double ry, double rz, double sx, double sy, double sz)
    {
        if (!m_Engine)
        {
            return false;
        }

        std::uint64_t uuid = parseUUID(uuidHex);
        float pos[3]   = { static_cast<float>(px), static_cast<float>(py), static_cast<float>(pz) };
        float rot[3]   = { static_cast<float>(rx), static_cast<float>(ry), static_cast<float>(rz) };
        float scale[3] = { static_cast<float>(sx), static_cast<float>(sy), static_cast<float>(sz) };

        bool success = m_Engine->SetEntityTransform(uuid, pos, rot, scale);
        if (success)
        {
            emit entityUpdated(uuidHex);
        }
        return success;
    }

    bool EditorApplication::setEntitySprite2D(const QString &uuidHex, double cr, double cg, double cb, double ca, double uvOx, double uvOy, double uvSx, double uvSy, bool visible)
    {
        if (!m_Engine)
        {
            return false;
        }

        std::uint64_t uuid = parseUUID(uuidHex);
        float color[4]    = { static_cast<float>(cr), static_cast<float>(cg), static_cast<float>(cb), static_cast<float>(ca) };
        float uvOffset[2] = { static_cast<float>(uvOx), static_cast<float>(uvOy) };
        float uvScale[2]  = { static_cast<float>(uvSx), static_cast<float>(uvSy) };

        bool success = m_Engine->SetEntitySprite2D(uuid, color, uvOffset, uvScale, visible);
        if (success)
        {
            emit entityUpdated(uuidHex);
        }
        return success;
    }

    bool EditorApplication::addEntityComponent(const QString &uuidHex, const QString &componentType)
    {
        if (!m_Engine)
        {
            return false;
        }

        std::uint64_t uuid = parseUUID(uuidHex);
        bool success = m_Engine->AddComponentToEntity(uuid, componentType.toUtf8().constData());
        if (success)
        {
            emit sceneEntitiesChanged();
            emit entityUpdated(uuidHex);
        }
        return success;
    }

    bool EditorApplication::removeEntityComponent(const QString &uuidHex, const QString &componentType)
    {
        if (!m_Engine)
        {
            return false;
        }

        std::uint64_t uuid = parseUUID(uuidHex);
        bool success = m_Engine->RemoveComponentFromEntity(uuid, componentType.toUtf8().constData());
        if (success)
        {
            emit sceneEntitiesChanged();
            emit entityUpdated(uuidHex);
        }
        return success;
    }

    bool EditorApplication::setEntityTexture(const QString &uuidHex, const QString &filePath)
    {
        if (!m_Engine)
        {
            return false;
        }

        std::uint64_t uuid = parseUUID(uuidHex);
        bool success = m_Engine->SetEntityTexture(uuid, filePath.toUtf8().constData());
        if (success)
        {
            emit entityUpdated(uuidHex);
        }
        return success;
    }

    QStringList EditorApplication::getAvailableTextures()
    {
        QStringList textures;

        const QStringList candidateDirs = {
            QCoreApplication::applicationDirPath() + "/Resources/Textures",
            QCoreApplication::applicationDirPath() + "/../Resources/Textures",
            "D:/Dev/TestDLL/Obscura/Resources/Textures"
        };

        for (const auto &dirPath : candidateDirs)
        {
            QDir dir(dirPath);
            if (dir.exists())
            {
                QStringList filters;
                filters << "*.jpg" << "*.jpeg" << "*.png" << "*.tga" << "*.bmp";
                QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files | QDir::Readable, QDir::Name);
                for (const auto &fileInfo : fileList)
                {
                    QString fileUrl = QUrl::fromLocalFile(fileInfo.absoluteFilePath()).toString();
                    if (!textures.contains(fileUrl))
                    {
                        textures.append(fileUrl);
                    }
                }
                if (!textures.isEmpty())
                {
                    break;
                }
            }
        }
        return textures;
    }

    QString EditorApplication::getTexturePathForSlot(int slot)
    {
        QStringList list = getAvailableTextures();
        if (slot >= 0 && slot < list.size())
        {
            return list.at(slot);
        }
        return QString();
    }

    void EditorApplication::clearLogs()
    {
        Obscura::Logger::ClearLogs();
    }
}
