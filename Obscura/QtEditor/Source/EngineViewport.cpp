#include "EngineViewport.hpp"
#include "EditorApplication.hpp"

#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGSimpleTextureNode>
#include <QtQuick/QSGRendererInterface>
#include <QtGui/QMouseEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QWheelEvent>
#include <QtCore/QDebug>

#if QT_CONFIG(vulkan)
#include <QtQuick/qsgtexture_platform.h>
#include <vulkan/vulkan.h>
#endif

namespace ObscuraEditor
{
    EngineViewport::EngineViewport(QQuickItem *parent)
        : QQuickItem(parent)
    {
        setFlag(ItemHasContents, true);
        setAcceptedMouseButtons(Qt::AllButtons);
        setAcceptHoverEvents(true);
        setFlag(ItemAcceptsInputMethod, true);

        m_ResizeDebounceTimer.setSingleShot(true);
        m_ResizeDebounceTimer.setInterval(120);
        connect(&m_ResizeDebounceTimer, &QTimer::timeout, this, [this]() {
            if (s_Application && m_PendingWidth > 0 && m_PendingHeight > 0)
            {
                m_PendingResize = true;
                update();
            }
        });

        if (s_Application)
        {
            connect(s_Application, &EditorApplication::frameRendered, this, &EngineViewport::onFrameReady, Qt::QueuedConnection);
        }
    }

    EngineViewport::~EngineViewport()
    {
        ClearTextureCache();
    }

    void EngineViewport::releaseResources()
    {
        ClearTextureCache();
    }

    void EngineViewport::itemChange(ItemChange change, const ItemChangeData &value)
    {
        if (change == ItemSceneChange && value.window)
        {
            connect(value.window, &QQuickWindow::sceneGraphInvalidated, this, [this]() {
                ClearTextureCache();
            }, Qt::DirectConnection);
        }
        QQuickItem::itemChange(change, value);
    }

    void EngineViewport::ClearTextureCache()
    {
        for (auto& entry : m_TextureCache)
        {
            delete entry.sgTexture;
        }
        m_TextureCache.clear();
    }

    void EngineViewport::SetApplicationInstance(EditorApplication* app)
    {
        s_Application = app;
    }

    void EngineViewport::onFrameReady()
    {
        update(); // Request scene graph paint node update
    }

    QSGNode *EngineViewport::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
    {
        auto *node = static_cast<QSGSimpleTextureNode *>(oldNode);

        if (!node)
        {
            node = new QSGSimpleTextureNode();
            node->setOwnsTexture(false);
        }

        if (m_PendingResize)
        {
            m_PendingResize = false;
            ClearTextureCache();

            // Safely recreate node so oldNode doesn't retain deleted texture pointers
            delete node;
            node = new QSGSimpleTextureNode();
            node->setOwnsTexture(false);

            if (s_Application)
            {
                s_Application->onViewportResized(m_PendingWidth, m_PendingHeight);
            }
        }

        if (s_Application && s_Application->getTransport().HasNewFrame())
        {
            if (s_Application->getTransport().IsGPUInterop())
            {
                m_LatestGpuHandle = s_Application->getTransport().AcquireGpuHandle();
            }
            else
            {
                m_LatestGpuHandle.isGPUInterop = false;
                m_LatestImage = s_Application->getTransport().AcquireFrame();
            }
        }

        if (!window())
        {
            return node;
        }

        QSGRendererInterface *rif = window()->rendererInterface();
        const QSGRendererInterface::GraphicsApi api = rif ? rif->graphicsApi() : QSGRendererInterface::Unknown;

        // Path 1: Zero-copy GPU Texture Interop with Vulkan SceneGraph
#if QT_CONFIG(vulkan)
        if (m_LatestGpuHandle.isGPUInterop && m_LatestGpuHandle.nativeHandle != nullptr && 
            m_LatestGpuHandle.width > 0 && m_LatestGpuHandle.height > 0 &&
            api == QSGRendererInterface::Vulkan)
        {
            // If viewport dimensions changed, clear old size textures
            if (!m_TextureCache.empty() && 
                (m_TextureCache.front().width != m_LatestGpuHandle.width || 
                 m_TextureCache.front().height != m_LatestGpuHandle.height))
            {
                ClearTextureCache();
                delete node;
                node = new QSGSimpleTextureNode();
                node->setOwnsTexture(false);
            }

            QSGTexture* targetSgTexture = nullptr;
            for (auto& entry : m_TextureCache)
            {
                if (entry.nativeHandle == m_LatestGpuHandle.nativeHandle)
                {
                    targetSgTexture = entry.sgTexture;
                    break;
                }
            }

            if (!targetSgTexture)
            {
                auto vkImg = reinterpret_cast<VkImage>(m_LatestGpuHandle.nativeHandle);
                auto vkLayout = static_cast<VkImageLayout>(m_LatestGpuHandle.layoutOrState);
                const QSize texSize(static_cast<int>(m_LatestGpuHandle.width), static_cast<int>(m_LatestGpuHandle.height));

                targetSgTexture = QNativeInterface::QSGVulkanTexture::fromNative(
                    vkImg,
                    vkLayout,
                    window(),
                    texSize,
                    QQuickWindow::TextureIsOpaque
                );

                if (targetSgTexture)
                {
                    m_TextureCache.push_back({
                        .nativeHandle = m_LatestGpuHandle.nativeHandle,
                        .width = m_LatestGpuHandle.width,
                        .height = m_LatestGpuHandle.height,
                        .sgTexture = targetSgTexture
                    });
                }
            }

            if (targetSgTexture)
            {
                if (node->ownsTexture())
                {
                    delete node->texture();
                    node->setOwnsTexture(false);
                }

                if (node->texture() != targetSgTexture)
                {
                    node->setTexture(targetSgTexture);
                }
                node->setRect(boundingRect());
                node->setFiltering(QSGTexture::Linear);
                node->markDirty(QSGNode::DirtyMaterial | QSGNode::DirtyGeometry);
                return node;
            }
        }
#endif

        // Path 2: CPU Image Texture Upload Fallback
        if (!m_LatestImage.isNull())
        {
            ClearTextureCache();

            if (node->ownsTexture())
            {
                delete node->texture();
                node->setOwnsTexture(false);
            }

            QSGTexture *texture = window()->createTextureFromImage(m_LatestImage, QQuickWindow::TextureIsOpaque);
            node->setOwnsTexture(true);
            node->setTexture(texture);
            node->setRect(boundingRect());
            node->setFiltering(QSGTexture::Linear);
            node->markDirty(QSGNode::DirtyMaterial | QSGNode::DirtyGeometry);
            return node;
        }

        // Placeholder dark frame
        if (!node->texture())
        {
            QImage placeholder(64, 64, QImage::Format_RGBA8888);
            placeholder.fill(QColor(18, 20, 24));
            QSGTexture *texture = window()->createTextureFromImage(placeholder);
            node->setOwnsTexture(true);
            node->setTexture(texture);
        }

        node->setRect(boundingRect());
        return node;
    }

    void EngineViewport::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
    {
        QQuickItem::geometryChange(newGeometry, oldGeometry);

        if (newGeometry.size() != oldGeometry.size())
        {
            if (newGeometry.width() < 16.0 || newGeometry.height() < 16.0)
            {
                return;
            }

            m_PendingWidth  = static_cast<std::uint32_t>(std::round(newGeometry.width()));
            m_PendingHeight = static_cast<std::uint32_t>(std::round(newGeometry.height()));

            // 1. Immediately update Camera Aspect Ratio so projection is always responsive and distortion-free
            if (s_Application)
            {
                s_Application->onViewportAspectResized(m_PendingWidth, m_PendingHeight);
            }

            // 2. Debounce heavy GPU image & framebuffer recreation until resize interaction finishes
            m_ResizeDebounceTimer.start(120);

            update(); // Request SceneGraph update to stretch current frame
        }
    }

    void EngineViewport::mousePressEvent(QMouseEvent *event)
    {
        forceActiveFocus();
        if (s_Application)
        {
            s_Application->handleMouseButton(static_cast<int>(event->button()), true, static_cast<float>(event->position().x()), static_cast<float>(event->position().y()));
        }
        event->accept();
    }

    void EngineViewport::mouseReleaseEvent(QMouseEvent *event)
    {
        if (s_Application)
        {
            s_Application->handleMouseButton(static_cast<int>(event->button()), false, static_cast<float>(event->position().x()), static_cast<float>(event->position().y()));
        }
        event->accept();
    }

    void EngineViewport::mouseMoveEvent(QMouseEvent *event)
    {
        if (s_Application)
        {
            s_Application->handleMouseMove(static_cast<float>(event->position().x()), static_cast<float>(event->position().y()));
        }
        event->accept();
    }

    void EngineViewport::wheelEvent(QWheelEvent *event)
    {
        event->accept();
    }

    void EngineViewport::keyPressEvent(QKeyEvent *event)
    {
        if (s_Application)
        {
            s_Application->handleKey(event->key(), true);
        }
        event->accept();
    }

    void EngineViewport::keyReleaseEvent(QKeyEvent *event)
    {
        if (s_Application)
        {
            s_Application->handleKey(event->key(), false);
        }
        event->accept();
    }
}
