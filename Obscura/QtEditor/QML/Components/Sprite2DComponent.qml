import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import Obscura.Editor 1.0
import ".."

ComponentBase {
    id: root
    title: "Sprite 2D"

    property real colorR: 1.0
    property real colorG: 1.0
    property real colorB: 1.0
    property real colorA: 1.0

    property int textureSlot: 0
    property bool useTexture: false
    property real uvOffsetX: 0.0
    property real uvOffsetY: 0.0
    property real uvScaleX: 1.0
    property real uvScaleY: 1.0
    property bool spriteVisible: true

    property var availableTextures: []

    signal spriteChanged()

    function refreshTextures() {
        root.availableTextures = EditorApp.getAvailableTextures();
    }

    Component.onCompleted: {
        root.refreshTextures();
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 8

        // Row 1: Visibility and Texture Mode Toggles
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
                    root.spriteVisible = checked;
                    root.spriteChanged();
                }
            }

            CheckBox {
                id: textureCheck
                text: "Enable Texture"
                checked: root.useTexture
                font.pixelSize: 11
                Material.accent: "#6c8dfa"
                onToggled: {
                    root.useTexture = checked;
                    root.spriteChanged();
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
                                root.spriteChanged();
                            }
                        }
                    }
                }
            }
        }

        // Row 3: Texture Preview & Selector
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6
            visible: root.useTexture

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Label {
                    text: "Texture"
                    Layout.preferredWidth: 65
                    color: "#a0aec0"
                    font.pixelSize: 11
                }

                ComboBox {
                    id: textureCombo
                    Layout.fillWidth: true
                    implicitHeight: 26
                    font.pixelSize: 11
                    model: {
                        var names = [];
                        for (var i = 0; i < root.availableTextures.length; ++i) {
                            var path = root.availableTextures[i];
                            var filename = path.substring(path.lastIndexOf('/') + 1);
                            names.push(`Slot ${i}: ${filename}`);
                        }
                        return names.length > 0 ? names : ["No textures found in Resources/Textures"];
                    }
                    currentIndex: (root.textureSlot >= 0 && root.textureSlot < root.availableTextures.length) ? root.textureSlot : 0

                    onActivated: (index) => {
                        root.textureSlot = index;
                        root.spriteChanged();
                    }
                }

                ToolButton {
                    text: "↻"
                    implicitHeight: 24
                    implicitWidth: 24
                    ToolTip.visible: hovered
                    ToolTip.text: "Reload Textures"
                    onClicked: root.refreshTextures()
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

                // Background checkerboard for transparent textures
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
                    source: (root.textureSlot >= 0 && root.textureSlot < root.availableTextures.length) ? root.availableTextures[root.textureSlot] : ""
                    asynchronous: true
                    sourceSize: Qt.size(256, 256)
                    smooth: true
                    mipmap: false
                }

                // Empty / Loading state placeholder
                ColumnLayout {
                    anchors.centerIn: parent
                    visible: texturePreviewImage.status !== Image.Ready
                    spacing: 4

                    BusyIndicator {
                        running: texturePreviewImage.status === Image.Loading
                        visible: running
                        Layout.alignment: Qt.AlignHCenter
                        implicitWidth: 20
                        implicitHeight: 20
                    }

                    Label {
                        text: texturePreviewImage.status === Image.Loading ? "Loading Preview..." : "No Texture Loaded"
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
                    visible: texturePreviewImage.status === Image.Ready

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
            label: "UV Offset"
            valueX: root.uvOffsetX
            valueY: root.uvOffsetY
            valueZ: 0.0
            onValuesChanged: (x, y, z) => {
                root.uvOffsetX = x;
                root.uvOffsetY = y;
                root.spriteChanged();
            }
        }

        Float3 {
            label: "UV Scale"
            valueX: root.uvScaleX
            valueY: root.uvScaleY
            valueZ: 1.0
            onValuesChanged: (x, y, z) => {
                root.uvScaleX = x;
                root.uvScaleY = y;
                root.spriteChanged();
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
                    onMoved: { root.colorR = value; root.spriteChanged(); }
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
                    onMoved: { root.colorG = value; root.spriteChanged(); }
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
                    onMoved: { root.colorB = value; root.spriteChanged(); }
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
                    onMoved: { root.colorA = value; root.spriteChanged(); }
                }
                Label { text: root.colorA.toFixed(2); font.pixelSize: 10; color: "#cbd5e0"; font.family: "Consolas" }
            }
        }
    }
}
