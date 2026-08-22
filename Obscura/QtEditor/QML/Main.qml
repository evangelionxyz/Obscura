import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import "Components"

ApplicationWindow {
    id: window
    width: 1600
    height: 900
    minimumWidth: 1024
    minimumHeight: 600
    visible: true
    title: qsTr("Obscura Engine Editor - [Unsaved Scene]")

    Material.theme: Material.Dark
    Material.accent: "#6c8dfa"
    Material.primary: "#1e222b"
    Material.background: "#16181d"

    menuBar: EditorMenuBar {}

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Main Editor Work Area (SplitView with Panels)
        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Vertical
            handle: Rectangle {
                implicitHeight: 3
                color: SplitHandle.hovered ? "#6c8dfa" : "#282c37"
            }

            SplitView {
                SplitView.fillWidth: true
                SplitView.fillHeight: true
                orientation: Qt.Horizontal
                handle: Rectangle {
                    implicitWidth: 3
                    color: SplitHandle.hovered ? "#6c8dfa" : "#282c37"
                }

                // Left Panel: Scene Outliner / Hierarchy
                Rectangle {
                    SplitView.preferredWidth: 260
                    SplitView.minimumWidth: 180
                    color: "#1a1d24"
                    border.color: "#282c37"
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        Rectangle {
                            Layout.fillWidth: true
                            height: 28
                            color: "#20242e"
                            border.color: "#2d3340"
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 8
                                anchors.rightMargin: 8
                                spacing: 4
                                Label {
                                    text: "Outliner"
                                    font.pixelSize: 12
                                    font.bold: true
                                    color: "#a0aec0"
                                }
                                Item { Layout.fillWidth: true }
                                ToolButton {
                                    text: "+ Add"
                                    implicitHeight: 18
                                    font.pixelSize: 12
                                    leftPadding: 6
                                    rightPadding: 6
                                    topPadding: 0
                                    bottomPadding: 0
                                }
                            }
                        }

                        ListView {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            model: ListModel {
                                ListElement { name: "Main Camera"; type: "Camera" }
                                ListElement { name: "Directional Light"; type: "Light" }
                                ListElement { name: "Sky Atmosphere"; type: "Atmosphere" }
                                ListElement { name: "Environment Mesh"; type: "StaticMesh" }
                                ListElement { name: "Post Process Volume"; type: "PostProcess" }
                            }
                            delegate: ItemDelegate {
                                width: parent.width
                                height: 22
                                leftPadding: 8
                                rightPadding: 8
                                topPadding: 0
                                bottomPadding: 0
                                contentItem: RowLayout {
                                    spacing: 6
                                    Label {
                                        text: "◆"
                                        font.pixelSize: 12
                                        color: "#6c8dfa"
                                    }
                                    Label {
                                        text: name
                                        font.pixelSize: 12
                                        color: "#e2e8f0"
                                        Layout.fillWidth: true
                                    }
                                    Label {
                                        text: type
                                        font.pixelSize: 12
                                        color: "#718096"
                                    }
                                }
                            }
                        }
                    }
                }

                // Center Panel: Viewport
                ViewportPanel {
                    id: viewportPanel
                    SplitView.fillWidth: true
                    SplitView.fillHeight: true
                }

                // Right Panel: Inspector / Properties
                Rectangle {
                    SplitView.preferredWidth: 300
                    SplitView.minimumWidth: 220
                    color: "#1a1d24"
                    border.color: "#282c37"
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        spacing: 0

                        Rectangle {
                            Layout.fillWidth: true
                            height: 28
                            color: "#20242e"
                            border.color: "#2d3340"
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 8
                                anchors.rightMargin: 8
                                Label {
                                    text: "Inspector"
                                    font.pixelSize: 12
                                    font.bold: true
                                    color: "#a0aec0"
                                }
                            }
                        }

                        ScrollView {
                            id: inspectorScroll
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                            ColumnLayout {
                                width: inspectorScroll.availableWidth
                                x: 0
                                spacing: 8

                                TransformComponent {}

                                Rectangle {
                                    Layout.fillWidth: true
                                    height: 1
                                    color: "#2d3340"
                                }
                                Label {
                                    text: "Engine Subsystems"
                                    font.bold: true
                                    font.pixelSize: 12
                                    color: "#cbd5e0"
                                }
                                Label {
                                    text: `RHI Backend: ${EditorApp.rhiName}`
                                    font.pixelSize: 12
                                    color: "#a0aec0"
                                }
                                Label {
                                    text: `Engine ABI: v${EditorApp.engineVersion}`
                                    font.pixelSize: 12
                                    color: "#a0aec0"
                                }
                            }
                        }
                    }
                }
            }

            // Bottom Panel: Console Log
            Rectangle {
                SplitView.preferredHeight: 140
                SplitView.minimumHeight: 60
                color: "#14171d"
                border.color: "#282c37"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Rectangle {
                        Layout.fillWidth: true
                        height: 28
                        color: "#1c2029"
                        border.color: "#2d3340"
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            Label {
                                text: "Console"
                                font.pixelSize: 12
                                font.bold: true
                                color: "#a0aec0"
                            }
                            Item { Layout.fillWidth: true }
                            ToolButton {
                                text: "Clear"
                                implicitHeight: 16
                                font.pixelSize: 12
                                leftPadding: 6
                                rightPadding: 6
                                topPadding: 0
                                bottomPadding: 0
                                onClicked: logModel.clear()
                            }
                        }
                    }

                    ListView {
                        id: logList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: ListModel {
                            id: logModel
                            ListElement { text: "[Host] Obscura Qt Editor application shell loaded." }
                            ListElement { text: "[Host] Initializing dynamic engine bridge..." }
                        }
                        delegate: Item {
                            width: logList.width
                            height: 18
                            Label {
                                anchors.left: parent.left
                                anchors.leftMargin: 8
                                anchors.verticalCenter: parent.verticalCenter
                                text: model.text
                                font.pixelSize: 12
                                font.family: "Consolas, monospace"
                                color: model.text.startsWith("[Error]") ? "#ff4b4b" : (model.text.startsWith("[Engine]") ? "#38ef7d" : "#94a3b8")
                            }
                        }

                        Connections {
                            target: EditorApp
                            function onLogMessage(message) {
                                logModel.append({ text: message });
                                logList.positionViewAtEnd();
                            }
                        }
                    }
                }
            }
        }

        // Bottom Status Bar
        Rectangle {
            Layout.fillWidth: true
            height: 26
            color: "#181b22"
            border.color: "#252a36"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 10

                Rectangle {
                    width: 6
                    height: 6
                    radius: 3
                    color: EditorApp.isEngineRunning ? "#38ef7d" : "#ff4b4b"
                }

                Label {
                    text: EditorApp.isEngineRunning ? "READY" : "STOPPED"
                    font.pixelSize: 12
                    font.bold: true
                    color: EditorApp.isEngineRunning ? "#38ef7d" : "#ff4b4b"
                }

                Rectangle { width: 1; height: 10; color: "#3a4152" }

                Label {
                    text: `Active RHI: ${EditorApp.rhiName}`
                    font.pixelSize: 12
                    color: "#94a3b8"
                }

                Rectangle { width: 1; height: 10; color: "#3a4152" }

                Label {
                    text: `Obscura Engine v${EditorApp.engineVersion}`
                    font.pixelSize: 12
                    color: "#94a3b8"
                }

                Item { Layout.fillWidth: true }

                Label {
                    text: `${EditorApp.fps.toFixed(1)} FPS (${EditorApp.frameCount} frames)`
                    font.pixelSize: 12
                    font.family: "Consolas, monospace"
                    color: "#6c8dfa"
                }
            }
        }
    }
}
