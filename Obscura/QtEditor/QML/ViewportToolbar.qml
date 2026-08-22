import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Top Viewport Toolbar
Rectangle {
    id: root
    property var targetViewport: null

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        spacing: 6

        ToolButton {
            text: "Lit"
            font.pixelSize: 12
            implicitHeight: 20
            leftPadding: 6
            rightPadding: 6
            topPadding: 1
            bottomPadding: 1
        }

        Rectangle {
            width: 1
            height: 14
            color: "#3a4152"
        }

        ToolButton {
            text: "Translate"
            font.pixelSize: 12
            implicitHeight: 20
            leftPadding: 6
            rightPadding: 6
            topPadding: 1
            bottomPadding: 1
            checked: true
        }

        ToolButton {
            text: "Rotate"
            font.pixelSize: 12
            implicitHeight: 20
            leftPadding: 6
            rightPadding: 6
            topPadding: 1
            bottomPadding: 1
        }

        ToolButton {
            text: "Scale"
            font.pixelSize: 12
            implicitHeight: 20
            leftPadding: 6
            rightPadding: 6
            topPadding: 1
            bottomPadding: 1
        }

        Item { Layout.fillWidth: true }

        Label {
            text: root.targetViewport ? `${Math.round(root.targetViewport.width)} × ${Math.round(root.targetViewport.height)}` : ""
            font.pixelSize: 12
            font.family: "Consolas, monospace"
            color: "#8e99b0"
        }
    }
}