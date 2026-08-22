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

    function updateFromEntity(uuid) {
        root.selectedEntityUuid = uuid;
        if (uuid && uuid.length > 0) {
            var ent = EditorApp.getEntity(uuid);
            root.selectedEntity = (ent && ent.uuid) ? ent : null;
        } else {
            root.selectedEntity = null;
        }
    }

    Connections {
        target: EditorApp
        function onEntityUpdated(uuid) {
            if (uuid === root.selectedEntityUuid) {
                var ent = EditorApp.getEntity(uuid);
                root.selectedEntity = (ent && ent.uuid) ? ent : null;
            }
        }
        function onSceneEntitiesChanged() {
            if (root.selectedEntityUuid && root.selectedEntityUuid.length > 0) {
                var ent = EditorApp.getEntity(root.selectedEntityUuid);
                if (ent && ent.uuid) {
                    root.selectedEntity = ent;
                } else {
                    root.selectedEntityUuid = "";
                    root.selectedEntity = null;
                }
            } else {
                root.selectedEntity = null;
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
                        uuid: (root.selectedEntity && root.selectedEntity.uuid) ? root.selectedEntity.uuid : ""
                        entityName: (root.selectedEntity && root.selectedEntity.name) ? root.selectedEntity.name : ""
                        parentUuid: (root.selectedEntity && root.selectedEntity.parentUuid) ? root.selectedEntity.parentUuid : ""
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
                        posX: (root.selectedEntity && root.selectedEntity.posX !== undefined) ? root.selectedEntity.posX : 0.0
                        posY: (root.selectedEntity && root.selectedEntity.posY !== undefined) ? root.selectedEntity.posY : 0.0
                        posZ: (root.selectedEntity && root.selectedEntity.posZ !== undefined) ? root.selectedEntity.posZ : 0.0
                        rotX: (root.selectedEntity && root.selectedEntity.rotX !== undefined) ? root.selectedEntity.rotX : 0.0
                        rotY: (root.selectedEntity && root.selectedEntity.rotY !== undefined) ? root.selectedEntity.rotY : 0.0
                        rotZ: (root.selectedEntity && root.selectedEntity.rotZ !== undefined) ? root.selectedEntity.rotZ : 0.0
                        scaleX: (root.selectedEntity && root.selectedEntity.scaleX !== undefined) ? root.selectedEntity.scaleX : 1.0
                        scaleY: (root.selectedEntity && root.selectedEntity.scaleY !== undefined) ? root.selectedEntity.scaleY : 1.0
                        scaleZ: (root.selectedEntity && root.selectedEntity.scaleZ !== undefined) ? root.selectedEntity.scaleZ : 1.0

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

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: "#2d3340"
                    }

                    // Sprite 2D Component
                    Sprite2DComponent {
                        id: spriteComp
                        visible: root.selectedEntity && root.selectedEntity.hasSprite2D
                        colorR: (root.selectedEntity && root.selectedEntity.colorR !== undefined) ? root.selectedEntity.colorR : 1.0
                        colorG: (root.selectedEntity && root.selectedEntity.colorG !== undefined) ? root.selectedEntity.colorG : 1.0
                        colorB: (root.selectedEntity && root.selectedEntity.colorB !== undefined) ? root.selectedEntity.colorB : 1.0
                        colorA: (root.selectedEntity && root.selectedEntity.colorA !== undefined) ? root.selectedEntity.colorA : 1.0
                        textureSlot: (root.selectedEntity && root.selectedEntity.textureSlot !== undefined) ? root.selectedEntity.textureSlot : 0
                        useTexture: (root.selectedEntity && root.selectedEntity.useTexture !== undefined) ? root.selectedEntity.useTexture : false
                        uvOffsetX: (root.selectedEntity && root.selectedEntity.uvOffsetX !== undefined) ? root.selectedEntity.uvOffsetX : 0.0
                        uvOffsetY: (root.selectedEntity && root.selectedEntity.uvOffsetY !== undefined) ? root.selectedEntity.uvOffsetY : 0.0
                        uvScaleX: (root.selectedEntity && root.selectedEntity.uvScaleX !== undefined) ? root.selectedEntity.uvScaleX : 1.0
                        uvScaleY: (root.selectedEntity && root.selectedEntity.uvScaleY !== undefined) ? root.selectedEntity.uvScaleY : 1.0
                        spriteVisible: (root.selectedEntity && root.selectedEntity.spriteVisible !== undefined) ? root.selectedEntity.spriteVisible : true

                        onSpriteChanged: {
                            if (root.selectedEntityUuid) {
                                EditorApp.setEntitySprite2D(
                                    root.selectedEntityUuid,
                                    colorR, colorG, colorB, colorA,
                                    textureSlot, useTexture,
                                    uvOffsetX, uvOffsetY,
                                    uvScaleX, uvScaleY,
                                    spriteVisible
                                );
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
