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
    property real contentLeftPadding: 12
    property real contentRightPadding: 8

    default property alias content: contentLayout.data

    // Header card
    Rectangle {
        Layout.fillWidth: true
        height: 24
        color: "#20242e"
        border.color: "#2d3340"
        border.width: 1
        radius: 0

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8
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
        }

        MouseArea {
            anchors.fill: parent
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
