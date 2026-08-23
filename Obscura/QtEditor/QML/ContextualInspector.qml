import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import Obscura.Editor 1.0
import "Components"

Rectangle {
    id: root
    color: "#1a1d24"
    border.color: "#282c37"
    border.width: 1

    property string selectedEntityUuid: ""
    property var selectedEntity: null

    onSelectedEntityUuidChanged: {
        root.updateFromEntity(root.selectedEntityUuid);
    }

    onSelectedEntityChanged: {
        root.syncComponents(root.selectedEntity);
    }

    function syncComponents(ent) {
        if (!ent || !ent.uuid) {
            idComp.loadFromEntity(null);
            transformComp.loadFromEntity(null);
            spriteComp.loadFromEntity(null);
            return;
        }
        idComp.loadFromEntity(ent);
        transformComp.loadFromEntity(ent);
        if (ent.hasSprite2D) {
            spriteComp.loadFromEntity(ent);
        }
    }

    function updateFromEntity(uuid) {
        root.selectedEntityUuid = uuid;
        if (uuid && uuid.length > 0) {
            var ent = EditorApp.getEntity(uuid);
            root.selectedEntity = (ent && ent.uuid) ? ent : null;
        } else {
            root.selectedEntity = null;
        }
        root.syncComponents(root.selectedEntity);
    }

    Connections {
        target: EditorApp
        function onEntityUpdated(uuid) {
            if (uuid === root.selectedEntityUuid) {
                var ent = EditorApp.getEntity(uuid);
                root.selectedEntity = (ent && ent.uuid) ? ent : null;
                root.syncComponents(root.selectedEntity);
            }
        }
        function onSceneEntitiesChanged() {
            if (root.selectedEntityUuid && root.selectedEntityUuid.length > 0) {
                var ent = EditorApp.getEntity(root.selectedEntityUuid);
                if (ent && ent.uuid) {
                    root.selectedEntity = ent;
                    root.syncComponents(ent);
                } else {
                    root.selectedEntityUuid = "";
                    root.selectedEntity = null;
                    root.syncComponents(null);
                }
            } else {
                root.selectedEntity = null;
                root.syncComponents(null);
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Inspector Header
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
                    text: (root.selectedEntity && root.selectedEntity.name) ? `Inspector (${root.selectedEntity.name})` : "Inspector"
                    font.pixelSize: 12
                    font.bold: true
                    color: "#a0aec0"
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }

                Rectangle {
                    width: typeBadgeLabel.contentWidth + 8
                    height: 16
                    radius: 3
                    color: (root.selectedEntity && root.selectedEntity.uuid) ? "#2b354f" : "#242833"
                    Label {
                        id: typeBadgeLabel
                        anchors.centerIn: parent
                        text: (root.selectedEntity && root.selectedEntity.uuid) ? (root.selectedEntity.hasSprite2D ? "SPRITE2D" : "ENTITY") : "NONE"
                        font.pixelSize: 9
                        font.bold: true
                        color: (root.selectedEntity && root.selectedEntity.uuid) ? "#6c8dfa" : "#718096"
                    }
                }
            }
        }

        // Inspector Scroll Area
        ScrollView {
            id: inspectorScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            ColumnLayout {
                width: inspectorScroll.availableWidth
                x: 0
                spacing: 8

                // 1. Entity Active View
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    visible: root.selectedEntity !== null && root.selectedEntity !== undefined && root.selectedEntity.uuid !== undefined

                    // ID / Identity Component
                    IDComponent {
                        id: idComp
                        onNameChanged: (newName) => {
                            EditorApp.setEntityName(root.selectedEntityUuid, newName);
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#2d3340"
                    }

                    // Transform Component
                    TransformComponent {
                        id: transformComp
                        onTransformChanged: {
                            if (root.selectedEntityUuid) {
                                EditorApp.setEntityTransform(
                                    root.selectedEntityUuid,
                                    posX, posY, posZ,
                                    rotX, rotY, rotZ,
                                    scaleX, scaleY, scaleZ
                                );
                            }
                        }
                    }

                    // Sprite 2D Component
                    ColumnLayout {
                        Layout.fillWidth: true
                        visible: root.selectedEntity && root.selectedEntity.hasSprite2D
                        spacing: 8

                        Rectangle {
                            Layout.fillWidth: true
                            height: 1
                            color: "#2d3340"
                        }

                        Sprite2DComponent {
                            id: spriteComp
                            onSpriteChanged: {
                                if (root.selectedEntityUuid) {
                                    EditorApp.setEntitySprite2D(
                                        root.selectedEntityUuid,
                                        colorR, colorG, colorB, colorA,
                                        uvOffsetX, uvOffsetY,
                                        uvScaleX, uvScaleY,
                                        spriteVisible
                                    );
                                }
                            }
                            onTextureSelected: (filePath) => {
                                if (root.selectedEntityUuid) {
                                    EditorApp.setEntityTexture(root.selectedEntityUuid, filePath);
                                }
                            }
                            onRemoveRequested: {
                                if (root.selectedEntityUuid) {
                                    EditorApp.removeEntityComponent(root.selectedEntityUuid, "Sprite2D");
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#2d3340"
                    }

                    // Add Component Button & Dropdown
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: 12
                        Layout.rightMargin: 12
                        Layout.topMargin: 4
                        Layout.bottomMargin: 4

                        Button {
                            id: addComponentBtn
                            text: "+ Add Component"
                            Layout.fillWidth: true
                            implicitHeight: 28
                            font.pixelSize: 11
                            font.bold: true
                            Material.background: "#20242e"
                            Material.foreground: "#6c8dfa"
                            onClicked: addComponentMenu.open()

                            Menu {
                                id: addComponentMenu
                                y: addComponentBtn.height + 2
                                width: addComponentBtn.width

                                MenuItem {
                                    text: "Sprite 2D"
                                    enabled: root.selectedEntity && !root.selectedEntity.hasSprite2D
                                    font.pixelSize: 11
                                    onTriggered: {
                                        if (root.selectedEntityUuid) {
                                            EditorApp.addEntityComponent(root.selectedEntityUuid, "Sprite2D");
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // 2. Empty Selection State
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 40
                    Layout.alignment: Qt.AlignHCenter
                    visible: root.selectedEntity === null || root.selectedEntity === undefined || !root.selectedEntity.uuid
                    spacing: 8

                    Label {
                        text: "No Object Selected"
                        font.pixelSize: 13
                        font.bold: true
                        color: "#718096"
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Label {
                        text: "Select an entity from Scene Outliner\nto view and edit its components."
                        font.pixelSize: 11
                        color: "#4a5568"
                        horizontalAlignment: Text.AlignHCenter
                        Layout.alignment: Qt.AlignHCenter
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#2d3340"
                }

                // Subsystems Info Card
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 12
                    Layout.rightMargin: 8
                    spacing: 4

                    Label {
                        text: "Engine Subsystems"
                        font.bold: true
                        font.pixelSize: 12
                        color: "#cbd5e0"
                    }
                    Label {
                        text: `RHI Backend: ${EditorApp.rhiName}`
                        font.pixelSize: 11
                        color: "#a0aec0"
                    }
                    Label {
                        text: `Engine ABI: v${EditorApp.engineVersion}`
                        font.pixelSize: 11
                        color: "#a0aec0"
                    }
                }
            }
        }
    }
}
