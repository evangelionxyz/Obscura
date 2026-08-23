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
    title: (EditorApp.currentScenePath && EditorApp.currentScenePath !== "") 
           ? `Obscura Engine Editor - [${EditorApp.currentScenePath}]` 
           : qsTr("Obscura Engine Editor - [Untitled Scene]")

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
                SceneOutliner {
                    id: sceneOutliner
                    SplitView.preferredWidth: 260
                    SplitView.minimumWidth: 180

                    onEntitySelected: (uuid, entityData) => {
                        contextualInspector.updateFromEntity(uuid);
                    }
                }

                // Center Panel: Viewport
                ViewportPanel {
                    id: viewportPanel
                    SplitView.fillWidth: true
                    SplitView.fillHeight: true
                }

                // Right Panel: Contextual Inspector / Properties
                ContextualInspector {
                    id: contextualInspector
                    SplitView.preferredWidth: 300
                    SplitView.minimumWidth: 220
                    selectedEntityUuid: sceneOutliner.selectedEntityUuid
                    selectedEntity: sceneOutliner.selectedEntity
                }
            }

            // Bottom Panel: Console Log
            Rectangle {
                id: consolePanel
                SplitView.preferredHeight: 240
                SplitView.minimumHeight: 60
                color: "#14171d"
                border.color: "#282c37"
                border.width: 1

                property int infoCount: 0
                property int warnCount: 0
                property int errorCount: 0
                property bool autoScroll: true

                function addLogMessage(msg, level) {
                    var lvl = (level !== undefined && level !== null) ? level : 2;
                    if (lvl >= 4) errorCount++;
                    else if (lvl === 3) warnCount++;
                    else infoCount++;

                    logModel.append({
                        text: msg,
                        level: lvl
                    });

                    if (autoScroll) {
                        logList.positionViewAtEnd();
                    }
                }

                function clearAllLogs() {
                    logModel.clear();
                    infoCount = 0;
                    warnCount = 0;
                    errorCount = 0;
                    EditorApp.clearLogs();
                }

                function isLevelVisible(lvl) {
                    if (lvl >= 4) return filterErrorBtn.checked;
                    if (lvl === 3) return filterWarnBtn.checked;
                    return filterInfoBtn.checked;
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    // Header Bar with Filter Buttons
                    Rectangle {
                        Layout.fillWidth: true
                        height: 28
                        color: "#1c2029"
                        border.color: "#2d3340"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            spacing: 6

                            Label {
                                text: "Console"
                                font.pixelSize: 12
                                font.bold: true
                                color: "#cbd5e1"
                            }

                            // Total log count badge
                            Rectangle {
                                implicitWidth: totalCountLabel.implicitWidth + 8
                                implicitHeight: 16
                                radius: 8
                                color: "#283042"
                                Label {
                                    id: totalCountLabel
                                    anchors.centerIn: parent
                                    text: logModel.count.toString()
                                    font.pixelSize: 10
                                    color: "#94a3b8"
                                }
                            }

                            Item { Layout.fillWidth: true }

                            // Info Filter Button
                            Rectangle {
                                id: filterInfoBtn
                                property bool checked: true
                                implicitWidth: infoRow.implicitWidth + 10
                                implicitHeight: 24
                                radius: 3
                                color: checked ? "#1e2c44" : (infoMouse.containsMouse ? "#202532" : "transparent")
                                border.color: checked ? "#3b82f6" : "#333b4d"
                                border.width: 1

                                RowLayout {
                                    id: infoRow
                                    anchors.centerIn: parent
                                    spacing: 4
                                    Label {
                                        text: "ℹ Info"
                                        font.pixelSize: 11
                                        font.bold: filterInfoBtn.checked
                                        color: filterInfoBtn.checked ? "#93c5fd" : "#64748b"
                                    }
                                    Rectangle {
                                        implicitWidth: Math.max(14, infoCountText.implicitWidth + 6)
                                        implicitHeight: 13
                                        radius: 6
                                        color: filterInfoBtn.checked ? "#3b82f6" : "#2d3748"
                                        Label {
                                            id: infoCountText
                                            anchors.centerIn: parent
                                            text: consolePanel.infoCount.toString()
                                            font.pixelSize: 10
                                            font.bold: true
                                            color: filterInfoBtn.checked ? "#ffffff" : "#a0aec0"
                                        }
                                    }
                                }

                                MouseArea {
                                    id: infoMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: filterInfoBtn.checked = !filterInfoBtn.checked
                                }
                            }

                            // Warning Filter Button
                            Rectangle {
                                id: filterWarnBtn
                                property bool checked: true
                                implicitWidth: warnRow.implicitWidth + 10
                                implicitHeight: 20
                                radius: 3
                                color: checked ? "#382c16" : (warnMouse.containsMouse ? "#202532" : "transparent")
                                border.color: checked ? "#f59e0b" : "#333b4d"
                                border.width: 1

                                RowLayout {
                                    id: warnRow
                                    anchors.centerIn: parent
                                    spacing: 4
                                    Label {
                                        text: "⚠ Warning"
                                        font.pixelSize: 11
                                        font.bold: filterWarnBtn.checked
                                        color: filterWarnBtn.checked ? "#fde047" : "#64748b"
                                    }
                                    Rectangle {
                                        implicitWidth: Math.max(14, warnCountText.implicitWidth + 6)
                                        implicitHeight: 13
                                        radius: 6
                                        color: filterWarnBtn.checked ? "#f59e0b" : "#2d3748"
                                        Label {
                                            id: warnCountText
                                            anchors.centerIn: parent
                                            text: consolePanel.warnCount.toString()
                                            font.pixelSize: 9
                                            font.bold: true
                                            color: filterWarnBtn.checked ? "#1a1306" : "#a0aec0"
                                        }
                                    }
                                }
                                MouseArea {
                                    id: warnMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: filterWarnBtn.checked = !filterWarnBtn.checked
                                }
                            }

                            // Error Filter Button
                            Rectangle {
                                id: filterErrorBtn
                                property bool checked: true
                                implicitWidth: errorRow.implicitWidth + 10
                                implicitHeight: 20
                                radius: 3
                                color: checked ? "#381a1c" : (errorMouse.containsMouse ? "#202532" : "transparent")
                                border.color: checked ? "#ef4444" : "#333b4d"
                                border.width: 1

                                RowLayout {
                                    id: errorRow
                                    anchors.centerIn: parent
                                    spacing: 4
                                    Label {
                                        text: "✖ Error"
                                        font.pixelSize: 11
                                        font.bold: filterErrorBtn.checked
                                        color: filterErrorBtn.checked ? "#fca5a5" : "#64748b"
                                    }
                                    Rectangle {
                                        implicitWidth: Math.max(14, errorCountText.implicitWidth + 6)
                                        implicitHeight: 13
                                        radius: 6
                                        color: filterErrorBtn.checked ? "#ef4444" : "#2d3748"
                                        Label {
                                            id: errorCountText
                                            anchors.centerIn: parent
                                            text: consolePanel.errorCount.toString()
                                            font.pixelSize: 9
                                            font.bold: true
                                            color: "#ffffff"
                                        }
                                    }
                                }
                                MouseArea {
                                    id: errorMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: filterErrorBtn.checked = !filterErrorBtn.checked
                                }
                            }

                            // Separator
                            Rectangle {
                                width: 1
                                height: 16
                                color: "#2d3340"
                            }

                            // Clear Button
                            ToolButton {
                                text: "Clear"
                                implicitHeight: 20
                                font.pixelSize: 11
                                leftPadding: 6
                                rightPadding: 6
                                topPadding: 0
                                bottomPadding: 0
                                onClicked: consolePanel.clearAllLogs()
                            }
                        }
                    }

                    // Log Message List View
                    ListView {
                        id: logList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: ListModel {
                            id: logModel
                        }

                        ScrollBar.vertical: ScrollBar {
                            policy: ScrollBar.AsNeeded
                        }

                        delegate: Rectangle {
                            id: logItem
                            width: logList.width
                            height: consolePanel.isLevelVisible(model.level) ? 20 : 0
                            visible: consolePanel.isLevelVisible(model.level)
                            color: logHover.containsMouse ? "#1c212c" : ((index % 2 === 0) ? "#14171d" : "#161920")

                            HoverHandler {
                                id: logHover
                            }

                            Rectangle {
                                width: 3
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                color: (model.level >= 4) ? "#ef4444" : ((model.level === 3) ? "#f59e0b" : ((model.level === 2) ? "#3b82f6" : "#64748b"))
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 8
                                anchors.rightMargin: 8
                                spacing: 6

                                Label {
                                    text: (model.level >= 5) ? "[FATAL]" : ((model.level === 4) ? "[ERROR]" : ((model.level === 3) ? "[WARN] " : ((model.level === 2) ? "[INFO] " : "[DEBUG]")))
                                    font.pixelSize: 11
                                    font.bold: true
                                    font.family: "Consolas, monospace"
                                    color: (model.level >= 4) ? "#ef4444" : ((model.level === 3) ? "#f59e0b" : ((model.level === 2) ? "#60a5fa" : "#64748b"))
                                }

                                Label {
                                    Layout.fillWidth: true
                                    text: model.text
                                    font.pixelSize: 11
                                    font.family: "Consolas, monospace"
                                    color: (model.level >= 4) ? "#fca5a5" : ((model.level === 3) ? "#fde047" : ((model.text.indexOf("[Engine]") !== -1) ? "#38ef7d" : "#e2e8f0"))
                                    elide: Text.ElideRight
                                }
                            }
                        }

                        Connections {
                            target: EditorApp
                            function onLogMessage(message, level) {
                                consolePanel.addLogMessage(message, level);
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
