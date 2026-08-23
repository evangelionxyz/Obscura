import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import Obscura.Editor 1.0

Rectangle {
    id: root
    color: "#1a1d24"
    border.color: "#282c37"
    border.width: 1

    property string selectedEntityUuid: ""
    property var selectedEntity: null

    signal entitySelected(string uuid, var entityData)

    function refresh() {
        if (!EditorApp.isEngineRunning) {
            entityModel.clear();
            root.selectedEntityUuid = "";
            root.selectedEntity = null;
            root.entitySelected("", null);
            return;
        }

        var list = EditorApp.getEntityList();
        entityModel.clear();
        var foundSelected = false;

        for (var i = 0; i < list.length; ++i) {
            entityModel.append(list[i]);
            if (list[i].uuid === root.selectedEntityUuid) {
                foundSelected = true;
                root.selectedEntity = list[i];
            }
        }

        if (!foundSelected) {
            if (list.length > 0) {
                root.selectEntity(list[0].uuid);
            } else {
                root.selectedEntityUuid = "";
                root.selectedEntity = null;
                root.entitySelected("", null);
            }
        }
    }

    function selectEntity(uuid) {
        root.selectedEntityUuid = uuid;
        var ent = uuid ? EditorApp.getEntity(uuid) : null;
        root.selectedEntity = (ent && ent.uuid) ? ent : null;
        root.entitySelected(uuid, root.selectedEntity);
    }

    ListModel {
        id: entityModel
    }

    Connections {
        target: EditorApp
        function onEngineInitialized(success) {
            if (success) {
                root.refresh();
            }
        }
        function onSceneEntitiesChanged() {
            root.refresh();
        }
        function onEntityUpdated(uuid) {
            if (uuid === root.selectedEntityUuid) {
                var ent = EditorApp.getEntity(uuid);
                root.selectedEntity = (ent && ent.uuid) ? ent : null;
            }
        }
    }

    Component.onCompleted: {
        root.refresh();
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Panel Header
        Rectangle {
            Layout.fillWidth: true
            height: 28
            color: "#20242e"
            border.color: "#2d3340"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 6

                Label {
                    text: "Scene Outliner"
                    font.pixelSize: 12
                    font.bold: true
                    color: "#a0aec0"
                }

                Rectangle {
                    width: countLabel.contentWidth + 8
                    height: 16
                    radius: 8
                    color: "#2d3748"
                    Label {
                        id: countLabel
                        anchors.centerIn: parent
                        text: entityModel.count.toString()
                        font.pixelSize: 10
                        font.bold: true
                        color: "#6c8dfa"
                    }
                }

                Item { Layout.fillWidth: true }

                ToolButton {
                    text: "+ Add"
                    implicitHeight: 20
                    font.pixelSize: 11
                    font.bold: true
                    leftPadding: 6
                    rightPadding: 6
                    topPadding: 0
                    bottomPadding: 0
                    onClicked: {
                        var newUuid = EditorApp.createEntity("Entity_" + (entityModel.count + 1), "");
                        if (newUuid && newUuid.length > 0) {
                            root.selectEntity(newUuid);
                        }
                    }
                }
            }
        }

        // Entity List
        ListView {
            id: entityListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: entityModel
            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            delegate: Rectangle {
                id: delegateRoot
                width: entityListView.width
                height: 26
                color: (model.uuid === root.selectedEntityUuid) ? "#2b354f" : (hoverArea.containsMouse ? "#20242e" : "transparent")
                border.color: (model.uuid === root.selectedEntityUuid) ? "#4a63a8" : "transparent"
                border.width: 1

                HoverHandler {
                    id: hoverArea
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onClicked: (mouse) => {
                        root.selectEntity(model.uuid);
                        if (mouse.button === Qt.RightButton) {
                            contextMenu.popup();
                        }
                    }
                }

                Menu {
                    id: contextMenu
                    MenuItem {
                        text: "Delete Entity"
                        onTriggered: {
                            EditorApp.destroyEntity(model.uuid);
                        }
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 6

                    Label {
                        text: model.hasSprite2D ? "◆" : "○"
                        font.pixelSize: 11
                        color: model.hasSprite2D ? "#6c8dfa" : "#718096"
                    }

                    Label {
                        text: model.name
                        font.pixelSize: 12
                        color: (model.uuid === root.selectedEntityUuid) ? "#ffffff" : "#e2e8f0"
                        font.bold: (model.uuid === root.selectedEntityUuid)
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }

                    Label {
                        text: model.type
                        font.pixelSize: 10
                        color: "#718096"
                    }
                }
            }
        }
    }
}
