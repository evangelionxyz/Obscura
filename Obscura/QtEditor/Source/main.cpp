#include "EditorApplication.hpp"
#include "EngineViewport.hpp"

#include <Obscura/Logger.hpp>

#include <QtGui/QGuiApplication>
#include <QtGui/QSurfaceFormat>
#include <QtGui/QVulkanInstance>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QQuickGraphicsDevice>
#include <QtQuick/QSGRendererInterface>
#include <QtQml/QQmlApplicationEngine>
#include <QtQml/QQmlContext>
#include <QtQuickControls2/QQuickStyle>
#include <QtCore/QDir>
#include <QtCore/QDebug>

#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <Windows.h>
    #include <cstdio>
#endif

using namespace ObscuraEditor;

int main(int argc, char *argv[])
{
#if defined(_WIN32)
    if (!AttachConsole(ATTACH_PARENT_PROCESS))
    {
        AllocConsole();
    }
    FILE* fpOut = nullptr;
    FILE* fpErr = nullptr;
    FILE* fpIn  = nullptr;
    freopen_s(&fpOut, "CONOUT$", "w", stdout);
    freopen_s(&fpErr, "CONOUT$", "w", stderr);
    freopen_s(&fpIn,  "CONIN$",  "r", stdin);
    std::ios::sync_with_stdio(true);
    SetConsoleTitleW(L"Obscura Editor Console");
#endif

    Obscura::Logger::Init();

    // Configure Qt Quick SceneGraph to use Vulkan for zero-copy GPU texture interop
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Vulkan);

    // Disable VSync on default surface format to uncap swapchain presentation
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setSwapInterval(0);
    QSurfaceFormat::setDefaultFormat(format);

    QGuiApplication app(argc, argv);
    app.setOrganizationName("ObscuraEngine");
    app.setApplicationName("Obscura Editor");
    app.setApplicationVersion("0.1.0");

    EditorApplication editorApp;

    // Initialize Engine & Vulkan RHI first to adopt existing GPU instance & device objects
    editorApp.initialize();

    QVulkanInstance qVulkanInstance;
    if (auto* rhi = editorApp.getRHI())
    {
        auto devObjs = rhi->GetVulkanDeviceObjects();
        if (devObjs.instance)
        {
            qVulkanInstance.setVkInstance(reinterpret_cast<VkInstance>(devObjs.instance));
            qVulkanInstance.create();
        }
    }

    int exitCode = 0;
    {
        QQmlApplicationEngine engine;
        engine.rootContext()->setContextProperty("EditorApp", &editorApp);

        engine.loadFromModule("Obscura.Editor", "Main");

        if (engine.rootObjects().isEmpty())
        {
            const QUrl fallbackUrl(QStringLiteral("qrc:/qt/qml/Obscura/Editor/QML/Main.qml"));
            qWarning() << "[QtEditor] Module load returned no root objects, trying URL:" << fallbackUrl;
            engine.load(fallbackUrl);
        }

        if (engine.rootObjects().isEmpty())
        {
            // Try direct file load fallback
            const QString directPath = QDir::currentPath() + "/Obscura/QtEditor/QML/Main.qml";
            qWarning() << "[QtEditor] Resource load empty, attempting local file fallback:" << directPath;
            engine.load(QUrl::fromLocalFile(directPath));
            if (engine.rootObjects().isEmpty())
            {
                qCritical() << "[QtEditor] Failed to load root QML file.";
                return -1;
            }
        }

        // Bind shared Vulkan Device to the Qt Quick SceneGraph window
        for (auto* obj : engine.rootObjects())
        {
            if (auto* window = qobject_cast<QQuickWindow*>(obj))
            {
                if (auto* rhi = editorApp.getRHI())
                {
                    auto devObjs = rhi->GetVulkanDeviceObjects();
                    if (devObjs.device && devObjs.physicalDevice)
                    {
                        window->setVulkanInstance(&qVulkanInstance);
                        window->setGraphicsDevice(QQuickGraphicsDevice::fromDeviceObjects(
                            reinterpret_cast<VkPhysicalDevice>(devObjs.physicalDevice),
                            reinterpret_cast<VkDevice>(devObjs.device),
                            static_cast<int>(devObjs.queueFamilyIndex),
                            static_cast<int>(devObjs.queueIndex)
                        ));
                    }
                }
            }
        }

        exitCode = app.exec();
    }

    editorApp.shutdown();

    Obscura::Logger::Shutdown();

    return exitCode;
}
