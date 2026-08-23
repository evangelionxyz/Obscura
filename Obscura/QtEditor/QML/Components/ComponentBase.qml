import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: root
    Layout.fillWidth: true
    spacing: 4

    property string title: "Component"
    property bool collapsible: true
    property bool collapsed: false
    property bool removable: true
    property real contentLeftPadding: 12
    property real contentRightPadding: 8

    signal removeRequested()

    default property alias content: contentLayout.data

    // Header card
    Rectangle {
        id: headerRect
        Layout.fillWidth: true
        height: 24
        color: headerHover.containsMouse ? "#262b37" : "#20242e"
        border.color: "#2d3340"
        border.width: 1
        radius: 0

        HoverHandler { id: headerHover }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 6
            spacing: 6

            Text {
                visible: root.collapsible
                text: root.collapsed ? "▶" : "▼"
                font.pixelSize: 9
                color: "#a0aec0"
            }

            Label {
                text: root.title
                font.bold: true
                font.pixelSize: 12
                color: "#cbd5e0"
                Layout.fillWidth: true
                verticalAlignment: Text.AlignVCenter
            }

            // Remove Button
            Rectangle {
                id: removeBtn
                visible: root.removable
                implicitWidth: 16
                implicitHeight: 16
                radius: 2
                color: removeBtnHover.containsMouse ? "#ef4444" : "transparent"

                HoverHandler { id: removeBtnHover }

                Text {
                    anchors.centerIn: parent
                    text: "✕"
                    font.pixelSize: 10
                    font.bold: true
                    color: removeBtnHover.containsMouse ? "#ffffff" : "#718096"
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        root.removeRequested();
                    }
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            anchors.rightMargin: root.removable ? 24 : 0
            enabled: root.collapsible
            cursorShape: Qt.PointingHandCursor
            onClicked: root.collapsed = !root.collapsed
        }
    }

    // Content container
    ColumnLayout {
        id: contentLayout
        Layout.fillWidth: true
        Layout.leftMargin: root.contentLeftPadding
        Layout.rightMargin: root.contentRightPadding
        spacing: 2
        visible: !root.collapsed
    }
}
