#pragma once

#include <QtQuick/QQuickItem>
#include <QtQuick/QSGSimpleTextureNode>
#include <QtGui/QImage>
#include <QtQml/qqmlregistration.h>
#include <Obscura/IRHI.hpp>

#include <vector>

namespace ObscuraEditor
{
    class EditorApplication;

    class EngineViewport : public QQuickItem
    {
        Q_OBJECT
        QML_ELEMENT

    public:
        explicit EngineViewport(QQuickItem *parent = nullptr);
        ~EngineViewport() override;

        static void SetApplicationInstance(EditorApplication* app);

    protected:
        QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) override;
        void releaseResources() override;
        void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
        void itemChange(ItemChange change, const ItemChangeData &value) override;

        void mousePressEvent(QMouseEvent *event) override;
        void mouseReleaseEvent(QMouseEvent *event) override;
        void mouseMoveEvent(QMouseEvent *event) override;
        void wheelEvent(QWheelEvent *event) override;
        void keyPressEvent(QKeyEvent *event) override;
        void keyReleaseEvent(QKeyEvent *event) override;

    public slots:
        void onFrameReady();

    private:
        void ClearTextureCache();

    private:
        struct TextureCacheEntry
        {
            void*         nativeHandle = nullptr;
            std::uint32_t width        = 0;
            std::uint32_t height       = 0;
            QSGTexture*   sgTexture    = nullptr;
        };

        static inline EditorApplication* s_Application = nullptr;
        Obscura::GPUTextureHandle        m_LatestGpuHandle{};
        QImage                           m_LatestImage;
        std::vector<TextureCacheEntry>   m_TextureCache;

        bool                             m_PendingResize = false;
        std::uint32_t                    m_PendingWidth = 0;
        std::uint32_t                    m_PendingHeight = 0;
    };
}
