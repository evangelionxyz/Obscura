import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ComponentBase {
    id: root
    title: "Identity & Hierarchy"
    removable: false

    property string uuid: "0x0000000000000000"
    property string entityName: "Entity"
    property string parentUuid: "0x0000000000000000"

    signal nameChanged(string newName)

    function loadFromEntity(entity) {
        if (!entity || !entity.uuid) {
            root.uuid = "0x0000000000000000";
            root.entityName = "Entity";
            root.parentUuid = "0x0000000000000000";
            return;
        }
        root.uuid = entity.uuid;
        root.entityName = entity.name !== undefined ? entity.name : "Entity";
        root.parentUuid = entity.parentUuid !== undefined ? entity.parentUuid : "0x0000000000000000";
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 6

        // Name Field (Editable)
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: "Name"
                Layout.preferredWidth: 65
                color: "#a0aec0"
                font.pixelSize: 11
                verticalAlignment: Text.AlignVCenter
            }

            Rectangle {
                Layout.fillWidth: true
                height: 22
                color: nameInput.activeFocus ? "#1a202c" : "#14171d"
                border.color: nameInput.activeFocus ? "#6c8dfa" : (nameHover.containsMouse ? "#3a4154" : "#242833")
                border.width: 1
                radius: 2

                HoverHandler { id: nameHover }

                TextInput {
                    id: nameInput
                    anchors.fill: parent
                    anchors.leftMargin: 6
                    anchors.rightMargin: 6
                    verticalAlignment: TextInput.AlignVCenter
                    color: "#e2e8f0"
                    selectionColor: "#6c8dfa"
                    selectedTextColor: "#ffffff"
                    font.pixelSize: 11
                    selectByMouse: true
                    text: root.entityName

                    onEditingFinished: {
                        if (text.trim().length > 0 && text !== root.entityName) {
                            root.nameChanged(text.trim());
                        }
                    }
                }
            }
        }

        // UUID Field (Read-only Hex)
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: "UUID"
                Layout.preferredWidth: 65
                color: "#a0aec0"
                font.pixelSize: 11
                verticalAlignment: Text.AlignVCenter
            }

            Rectangle {
                Layout.fillWidth: true
                height: 22
                color: "#11141a"
                border.color: "#242833"
                border.width: 1
                radius: 2

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 6
                    anchors.rightMargin: 6
                    spacing: 4

                    Text {
                        text: root.uuid
                        Layout.fillWidth: true
                        font.pixelSize: 10
                        font.family: "Consolas, monospace"
                        color: "#6c8dfa"
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    Label {
                        text: "HASH64"
                        font.pixelSize: 8
                        font.bold: true
                        color: "#4a5568"
                        padding: 2
                    }
                }
            }
        }

        // Parent UUID Field (Read-only Hex)
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Label {
                text: "Parent"
                Layout.preferredWidth: 65
                color: "#a0aec0"
                font.pixelSize: 11
                verticalAlignment: Text.AlignVCenter
            }

            Rectangle {
                Layout.fillWidth: true
                height: 22
                color: "#11141a"
                border.color: "#242833"
                border.width: 1
                radius: 2

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 6
                    anchors.rightMargin: 6
                    spacing: 4

                    Text {
                        text: (root.parentUuid === "0x0000000000000000" || root.parentUuid === "") ? "Root (0x0)" : root.parentUuid
                        Layout.fillWidth: true
                        font.pixelSize: 10
                        font.family: "Consolas, monospace"
                        color: (root.parentUuid === "0x0000000000000000" || root.parentUuid === "") ? "#718096" : "#cbd5e0"
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                }
            }
        }
    }
}
