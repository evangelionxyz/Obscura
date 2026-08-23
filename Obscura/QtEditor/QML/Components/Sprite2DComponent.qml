import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs
import Obscura.Editor 1.0
import ".."

ComponentBase {
    id: root
    title: "Sprite 2D"
    removable: true

    property real colorR: 1.0
    property real colorG: 1.0
    property real colorB: 1.0
    property real colorA: 1.0

    property string textureHandle: "0x0000000000000000"
    property string texturePath: ""
    property int textureState: 0 // 0=Unloaded, 1=Loading, 2=Ready, 3=Failed

    property real uvOffsetX: 0.0
    property real uvOffsetY: 0.0
    property real uvScaleX: 1.0
    property real uvScaleY: 1.0
    property bool spriteVisible: true

    property bool updating: false

    signal spriteChanged()
    signal textureSelected(string filePath)

    function loadFromEntity(entity) {
        if (!entity) return;
        root.updating = true;
        root.colorR = (entity.colorR !== undefined) ? entity.colorR : 1.0;
        root.colorG = (entity.colorG !== undefined) ? entity.colorG : 1.0;
        root.colorB = (entity.colorB !== undefined) ? entity.colorB : 1.0;
        root.colorA = (entity.colorA !== undefined) ? entity.colorA : 1.0;
        root.textureHandle = (entity.textureHandle !== undefined) ? entity.textureHandle : "0x0000000000000000";
        root.texturePath = (entity.texturePath !== undefined) ? entity.texturePath : "";
        root.textureState = (entity.textureState !== undefined) ? entity.textureState : 0;
        root.uvOffsetX = (entity.uvOffsetX !== undefined) ? entity.uvOffsetX : 0.0;
        root.uvOffsetY = (entity.uvOffsetY !== undefined) ? entity.uvOffsetY : 0.0;
        root.uvScaleX = (entity.uvScaleX !== undefined) ? entity.uvScaleX : 1.0;
        root.uvScaleY = (entity.uvScaleY !== undefined) ? entity.uvScaleY : 1.0;
        root.spriteVisible = (entity.spriteVisible !== undefined) ? entity.spriteVisible : true;

        uvOffsetFloat3.setValues(root.uvOffsetX, root.uvOffsetY, 0.0);
        uvScaleFloat3.setValues(root.uvScaleX, root.uvScaleY, 1.0);

        root.updating = false;
    }

    function emitSpriteChanged() {
        if (!root.updating) {
            root.spriteChanged();
        }
    }

    FileDialog {
        id: textureFileDialog
        title: "Select Sprite Texture"
        nameFilters: ["Image files (*.png *.jpg *.jpeg *.bmp *.tga)", "All files (*)"]
        onAccepted: {
            var selected = selectedFile.toString();
            if (selected.startsWith("file:///")) {
                selected = selected.substring(8);
            } else if (selected.startsWith("file://")) {
                selected = selected.substring(7);
            }
            root.texturePath = selected;
            root.textureSelected(selected);
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 8

        // Row 1: Visibility Toggle & State Badge
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            CheckBox {
                id: visibleCheck
                text: "Visible"
                checked: root.spriteVisible
                font.pixelSize: 11
                Material.accent: "#6c8dfa"
                onToggled: {
                    if (root.updating) return;
                    root.spriteVisible = checked;
                    root.emitSpriteChanged();
                }
            }

            Item { Layout.fillWidth: true }

            // Texture status badge
            Rectangle {
                implicitHeight: 18
                implicitWidth: statusLabel.contentWidth + 10
                radius: 3
                color: {
                    if (!root.texturePath || root.texturePath.length === 0) return "#242833";
                    if (root.textureState === 2) return "#1e3a29";
                    if (root.textureState === 1) return "#3a301e";
                    if (root.textureState === 3) return "#3a1e1e";
                    return "#242833";
                }
                border.color: {
                    if (!root.texturePath || root.texturePath.length === 0) return "#2d3340";
                    if (root.textureState === 2) return "#38ef7d";
                    if (root.textureState === 1) return "#eab308";
                    if (root.textureState === 3) return "#ef4444";
                    return "#2d3340";
                }
                border.width: 1

                Label {
                    id: statusLabel
                    anchors.centerIn: parent
                    text: {
                        if (!root.texturePath || root.texturePath.length === 0) return "NO TEXTURE (TINT)";
                        if (root.textureState === 2) return "TEXTURE READY";
                        if (root.textureState === 1) return "LOADING...";
                        if (root.textureState === 3) return "LOAD FAILED";
                        return "CUSTOM TEXTURE";
                    }
                    font.pixelSize: 9
                    font.bold: true
                    color: {
                        if (!root.texturePath || root.texturePath.length === 0) return "#718096";
                        if (root.textureState === 2) return "#38ef7d";
                        if (root.textureState === 1) return "#eab308";
                        if (root.textureState === 3) return "#ef4444";
                        return "#cbd5e0";
                    }
                }
            }
        }

        // Row 2: Tint Color Swatch & Interactive Picker
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Label {
                    text: "Tint Color"
                    Layout.preferredWidth: 65
                    color: "#a0aec0"
                    font.pixelSize: 11
                }

                // Interactive Color Swatch
                Rectangle {
                    id: colorSwatch
                    Layout.preferredWidth: 40
                    height: 22
                    radius: 3
                    color: Qt.rgba(root.colorR, root.colorG, root.colorB, root.colorA)
                    border.color: swatchHover.containsMouse ? "#6c8dfa" : "#4a5568"
                    border.width: 1

                    HoverHandler { id: swatchHover }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: colorPickerPopup.open()
                    }
                }

                // Hex code display
                Rectangle {
                    Layout.fillWidth: true
                    height: 22
                    color: "#14171d"
                    border.color: "#242833"
                    border.width: 1
                    radius: 2

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 6
                        anchors.rightMargin: 6

                        Text {
                            text: {
                                var r = Math.round(root.colorR * 255).toString(16).padStart(2, '0');
                                var g = Math.round(root.colorG * 255).toString(16).padStart(2, '0');
                                var b = Math.round(root.colorB * 255).toString(16).padStart(2, '0');
                                var a = Math.round(root.colorA * 255).toString(16).padStart(2, '0');
                                return ("#" + r + g + b + a).toUpperCase();
                            }
                            font.pixelSize: 10
                            font.family: "Consolas, monospace"
                            color: "#cbd5e0"
                        }
                    }
                }

                Button {
                    text: "Edit"
                    implicitHeight: 22
                    font.pixelSize: 10
                    onClicked: colorPickerPopup.open()
                }
            }

            // Quick Color Palette Presets
            RowLayout {
                Layout.fillWidth: true
                spacing: 4
                Layout.leftMargin: 73

                Repeater {
                    model: ["#ffffff", "#ff4b4b", "#38ef7d", "#3b82f6", "#eab308", "#06b6d4", "#a855f7", "#64748b"]
                    delegate: Rectangle {
                        width: 14
                        height: 14
                        radius: 2
                        color: modelData
                        border.color: "#1e222b"
                        border.width: 1

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                var c = Qt.color(modelData);
                                root.colorR = c.r;
                                root.colorG = c.g;
                                root.colorB = c.b;
                                root.colorA = 1.0;
                                root.emitSpriteChanged();
                            }
                        }
                    }
                }
            }
        }

        // Row 3: Texture File Picker & Preview Card
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Label {
                    text: "Texture"
                    Layout.preferredWidth: 65
                    color: "#a0aec0"
                    font.pixelSize: 11
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 24
                    color: "#14171d"
                    border.color: "#242833"
                    border.width: 1
                    radius: 2
                    clip: true

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 6
                        anchors.rightMargin: 6

                        Text {
                            text: root.texturePath.length > 0 ? root.texturePath : "None (Solid Color Tint)"
                            font.pixelSize: 10
                            font.family: "Consolas, monospace"
                            color: root.texturePath.length > 0 ? "#cbd5e0" : "#718096"
                            elide: Text.ElideMiddle
                            Layout.fillWidth: true
                        }
                    }
                }

                Button {
                    text: "Browse..."
                    implicitHeight: 24
                    font.pixelSize: 10
                    onClicked: textureFileDialog.open()
                }

                ToolButton {
                    visible: root.texturePath.length > 0
                    text: "✕"
                    implicitHeight: 24
                    implicitWidth: 24
                    font.pixelSize: 10
                    ToolTip.visible: hovered
                    ToolTip.text: "Clear Texture"
                    onClicked: {
                        root.texturePath = "";
                        root.textureSelected("");
                    }
                }
            }

            // Texture Thumbnail Preview Card
            Rectangle {
                Layout.fillWidth: true
                height: 110
                color: "#11141a"
                border.color: "#242833"
                border.width: 1
                radius: 4
                clip: true

                // Background checkerboard pattern for transparent textures
                Rectangle {
                    anchors.fill: parent
                    color: "#151820"
                }

                Image {
                    id: texturePreviewImage
                    anchors.centerIn: parent
                    width: parent.width - 12
                    height: parent.height - 12
                    fillMode: Image.PreserveAspectFit
                    source: root.texturePath.length > 0 ? ("file:///" + root.texturePath) : ""
                    asynchronous: true
                    sourceSize: Qt.size(256, 256)
                    smooth: true
                    mipmap: false
                }

                // Empty / Loading state placeholder
                ColumnLayout {
                    anchors.centerIn: parent
                    visible: root.texturePath.length === 0 || texturePreviewImage.status !== Image.Ready
                    spacing: 4

                    BusyIndicator {
                        running: texturePreviewImage.status === Image.Loading
                        visible: running
                        Layout.alignment: Qt.AlignHCenter
                        implicitWidth: 20
                        implicitHeight: 20
                    }

                    Label {
                        text: {
                            if (root.texturePath.length === 0) return "No Texture (Renders Solid Tint)";
                            if (texturePreviewImage.status === Image.Loading) return "Loading Texture...";
                            if (texturePreviewImage.status === Image.Error) return "Failed to Load Image";
                            return "Ready";
                        }
                        font.pixelSize: 10
                        color: "#718096"
                        Layout.alignment: Qt.AlignHCenter
                    }
                }

                // File name & Dimensions pill
                Rectangle {
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 4
                    height: 16
                    width: textureMetaLabel.contentWidth + 8
                    color: "#cc1a1d24"
                    radius: 2
                    visible: texturePreviewImage.status === Image.Ready && root.texturePath.length > 0

                    Label {
                        id: textureMetaLabel
                        anchors.centerIn: parent
                        text: `${texturePreviewImage.sourceSize.width}x${texturePreviewImage.sourceSize.height}`
                        font.pixelSize: 9
                        font.family: "Consolas, monospace"
                        color: "#94a3b8"
                    }
                }
            }
        }

        // Row 4: UV Coordinates (Offset & Scale)
        Float3 {
            id: uvOffsetFloat3
            label: "UV Offset"
            valueX: root.uvOffsetX
            valueY: root.uvOffsetY
            valueZ: 0.0
            onValuesChanged: (x, y, z) => {
                if (root.updating) return;
                root.uvOffsetX = x;
                root.uvOffsetY = y;
                root.emitSpriteChanged();
            }
        }

        Float3 {
            id: uvScaleFloat3
            label: "UV Scale"
            valueX: root.uvScaleX
            valueY: root.uvScaleY
            valueZ: 1.0
            onValuesChanged: (x, y, z) => {
                if (root.updating) return;
                root.uvScaleX = x;
                root.uvScaleY = y;
                root.emitSpriteChanged();
            }
        }
    }

    // Interactive RGBA Color Sliders Popup
    Popup {
        id: colorPickerPopup
        width: 300
        height: 300
        modal: false
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        x: (parent.width - width) / 2
        y: 40

        background: Rectangle {
            color: "#1e222b"
            border.color: "#3a4154"
            border.width: 1
            radius: 4
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 0

            Label {
                text: "Edit RGBA Tint"
                font.bold: true
                font.pixelSize: 12
                color: "#e2e8f0"
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#2d3340"
            }

            // Red slider
            RowLayout {
                Layout.fillWidth: true
                Label { text: "R"; color: "#e53935"; font.bold: true; Layout.preferredWidth: 15 }
                Slider {
                    Layout.fillWidth: true
                    from: 0.0; to: 1.0
                    value: root.colorR
                    onMoved: { root.colorR = value; root.emitSpriteChanged(); }
                }
                Label { text: root.colorR.toFixed(2); font.pixelSize: 10; color: "#cbd5e0"; font.family: "Consolas" }
            }

            // Green slider
            RowLayout {
                Layout.fillWidth: true
                Label { text: "G"; color: "#43a047"; font.bold: true; Layout.preferredWidth: 15 }
                Slider {
                    Layout.fillWidth: true
                    from: 0.0; to: 1.0
                    value: root.colorG
                    onMoved: { root.colorG = value; root.emitSpriteChanged(); }
                }
                Label { text: root.colorG.toFixed(2); font.pixelSize: 10; color: "#cbd5e0"; font.family: "Consolas" }
            }

            // Blue slider
            RowLayout {
                Layout.fillWidth: true
                Label { text: "B"; color: "#1e88e5"; font.bold: true; Layout.preferredWidth: 15 }
                Slider {
                    Layout.fillWidth: true
                    from: 0.0; to: 1.0
                    value: root.colorB
                    onMoved: { root.colorB = value; root.emitSpriteChanged(); }
                }
                Label { text: root.colorB.toFixed(2); font.pixelSize: 10; color: "#cbd5e0"; font.family: "Consolas" }
            }

            // Alpha slider
            RowLayout {
                Layout.fillWidth: true
                Label { text: "A"; color: "#cbd5e0"; font.bold: true; Layout.preferredWidth: 15 }
                Slider {
                    Layout.fillWidth: true
                    from: 0.0; to: 1.0
                    value: root.colorA
                    onMoved: { root.colorA = value; root.emitSpriteChanged(); }
                }
                Label { text: root.colorA.toFixed(2); font.pixelSize: 10; color: "#cbd5e0"; font.family: "Consolas" }
            }
        }
    }
}
