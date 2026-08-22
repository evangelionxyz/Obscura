import QtQuick
import QtQuick.Controls

MenuBar {
    id: root
    implicitHeight: 28
    spacing: 1

    delegate: MenuBarItem {
        id: menuBarItem
        implicitHeight: 24
        topPadding: 0
        bottomPadding: 0
        leftPadding: 4
        rightPadding: 4
        font.pixelSize: 12

        contentItem: Text {
            text: menuBarItem.text
            font: menuBarItem.font
            color: menuBarItem.highlighted ? "#ffffff" : "#c0c6d4"
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            color: menuBarItem.highlighted ? "#3a4154" : "transparent"
        }
    }

    background: Rectangle {
        color: "#16181d"
        border.color: "#282c37"
        border.width: 1
    }

    Menu {
        title: qsTr("File")
        font.pixelSize: 12

        Action {
            text: qsTr("New Scene")
            shortcut: StandardKey.New
            onTriggered: console.log("[UI] New Scene triggered")
        }
        Action {
            text: qsTr("Open Scene...")
            shortcut: StandardKey.Open
            onTriggered: console.log("[UI] Open Scene triggered")
        }
        Action {
            text: qsTr("Save")
            shortcut: StandardKey.Save
            onTriggered: console.log("[UI] Save Scene triggered")
        }
        Action {
            text: qsTr("Save &As...")
            shortcut: StandardKey.SaveAs
            onTriggered: console.log("[UI] Save As triggered")
        }
        MenuSeparator {}
        Action {
            text: qsTr("Exit")
            shortcut: StandardKey.Quit
            onTriggered: EditorApp.requestExit()
        }
    }

    Menu {
        title: qsTr("Edit")
        font.pixelSize: 12

        Action {
            text: qsTr("Undo")
            shortcut: StandardKey.Undo
        }
        Action {
            text: qsTr("Redo")
            shortcut: StandardKey.Redo
        }
        MenuSeparator {}
        Action {
            text: qsTr("Cu&t")
            shortcut: StandardKey.Cut
        }
        Action {
            text: qsTr("Copy")
            shortcut: StandardKey.Copy
        }
        Action {
            text: qsTr("Paste")
            shortcut: StandardKey.Paste
        }
    }

    Menu {
        title: qsTr("View")
        font.pixelSize: 12

        Action {
            text: qsTr("Toggle Fullscreen")
            shortcut: "F11"
        }
        Action {
            text: qsTr("Reset Layout")
        }
    }

    Menu {
        title: qsTr("Engine")
        font.pixelSize: 12

        Action {
            text: qsTr("VSync (Lock ~60 FPS)")
            checkable: true
            checked: EditorApp.vsyncEnabled
            onTriggered: EditorApp.vsyncEnabled = checked
        }
        MenuSeparator {}
        Action {
            text: qsTr("Reload Plugins")
            onTriggered: console.log("[UI] Reload plugins triggered")
        }
        Action {
            text: qsTr("Recompile Shaders")
            onTriggered: console.log("[UI] Recompile shaders triggered")
        }
    }

    Menu {
        title: qsTr("Help")
        font.pixelSize: 12

        Action {
            text: qsTr("About Obscura Engine...")
            onTriggered: console.log("[UI] About dialog triggered")
        }
    }
}
