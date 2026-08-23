import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Obscura.Editor 1.0

Rectangle {
    id: root
    color: "#121418"
    clip: true

    // Top Viewport Toolbar
    ViewportToolbar  {
        id: toolbar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 28
        color: "#1a1d24"
        border.color: "#282c37"
        border.width: 1
        z: 10
        targetViewport: viewport
    }

    // Engine Render Surface
    EngineViewport {
        id: viewport
        anchors.top: toolbar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        focus: true
    }

    // HUD Overlay
    Rectangle {
        anchors.top: toolbar.bottom
        anchors.left: parent.left
        anchors.margins: 8
        width: hudLayout.implicitWidth + 14
        height: hudLayout.implicitHeight + 10
        color: "#00000000"
        border.width: 0
        z: 20

        ColumnLayout {
            id: hudLayout
            anchors.centerIn: parent
            spacing: 2

            RowLayout {
                spacing: 6
                Rectangle {
                    width: 6
                    height: 6
                    color: EditorApp.isEngineRunning ? "#38ef7d" : "#ff4b4b"
                }
                Label {
                    text: EditorApp.isEngineRunning ? "Engine Online" : "Engine Offline"
                    font.pixelSize: 12
                    font.bold: true
                    color: "#ffffff"
                }
            }

            Label {
                text: `FPS: ${EditorApp.fps.toFixed(1)}  |  Frame: ${EditorApp.frameCount}`
                font.pixelSize: 12
                font.family: "Consolas, monospace"
                color: "#11998e"
            }

            Label {
                text: `Frame Time: ${EditorApp.frameTime} ms`
                font.pixelSize: 12
                font.family: "Consolas, monospace"
                color: "#11998e"
            }

            Label {
                text: `RHI: ${EditorApp.rhiName}`
                font.pixelSize: 12
                font.family: "Consolas, monospace"
                color: "#a0aec0"
            }

            Label {
                text: `Engine: v${EditorApp.engineVersion}`
                font.pixelSize: 12
                font.family: "Consolas, monospace"
                color: "#718096"
            }
        }
    }
}
