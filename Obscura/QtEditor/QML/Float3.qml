import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    id: root
    Layout.fillWidth: true
    spacing: 4

    property string label: "Vector3"
    property real valueX: 0.0
    property real valueY: 0.0
    property real valueZ: 0.0
    property real labelWidth: 55
    property real dragSpeed: 0.1
    property bool readOnly: false

    signal valuesChanged(real x, real y, real z)

    Label {
        text: root.label
        Layout.preferredWidth: root.labelWidth
        color: "#a0aec0"
        font.pixelSize: 11
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
    }

    // Component for each XYZ axis channel
    component AxisChannel: Rectangle {
        id: channel
        Layout.fillWidth: true
        implicitHeight: 22
        color: inputField.activeFocus ? "#1a202c" : "#14171d"
        border.color: inputField.activeFocus ? channelColor : (hoverArea.containsMouse ? "#3a4154" : "#242833")
        border.width: 1
        radius: 2

        required property string axisLabel
        required property color channelColor
        property real value: 0.0
        signal valueModified(real newVal)

        HoverHandler {
            id: hoverArea
        }

        // Color-coded axis badge (Draggable handle)
        Rectangle {
            id: badge
            width: 15
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            color: channel.channelColor
            radius: 2

            // Flatten right radius to merge with input field
            Rectangle {
                width: 3
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                color: channel.channelColor
            }

            Text {
                anchors.centerIn: parent
                text: channel.axisLabel
                color: "#ffffff"
                font.pixelSize: 9
                font.bold: true
            }

            MouseArea {
                id: dragArea
                anchors.fill: parent
                cursorShape: Qt.SizeHorCursor
                preventStealing: true
                hoverEnabled: true

                property real startX: 0
                property real startVal: 0
                property bool isDragging: false

                onPressed: (mouse) => {
                    var globalPos = mapToItem(null, mouse.x, mouse.y);
                    startX = globalPos.x;
                    startVal = channel.value;
                    isDragging = true;
                }

                onPositionChanged: (mouse) => {
                    if (pressed && isDragging) {
                        var globalPos = mapToItem(null, mouse.x, mouse.y);
                        var delta = globalPos.x - startX;
                        var step = (mouse.modifiers & Qt.ShiftModifier) ? 0.01 : ((mouse.modifiers & Qt.ControlModifier) ? 1.0 : root.dragSpeed);
                        var newVal = Math.round((startVal + delta * step) * 100) / 100;
                        channel.valueModified(newVal);
                    }
                }

                onReleased: {
                    isDragging = false;
                }

                onCanceled: {
                    isDragging = false;
                }
            }

            // Global cursor overlay while dragging
            MouseArea {
                id: dragOverlay
                parent: root.Window.window ? root.Window.window.contentItem : null
                anchors.fill: parent
                visible: dragArea.isDragging
                cursorShape: Qt.SizeHorCursor
                z: 99999
            }
        }

        // Borderless numerical input field
        TextInput {
            id: inputField
            anchors.left: badge.right
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.leftMargin: 4
            anchors.rightMargin: 4
            verticalAlignment: TextInput.AlignVCenter
            color: "#e2e8f0"
            selectionColor: "#6c8dfa"
            selectedTextColor: "#ffffff"
            font.pixelSize: 11
            font.family: "Consolas, monospace"
            selectByMouse: true
            readOnly: root.readOnly
            text: Number(channel.value).toFixed(2)

            onEditingFinished: {
                var val = parseFloat(text);
                if (!isNaN(val)) {
                    channel.valueModified(val);
                } else {
                    text = Number(channel.value).toFixed(2);
                }
            }
        }
    }

    // X Channel (Red)
    AxisChannel {
        axisLabel: "X"
        channelColor: "#e53935"
        value: root.valueX
        onValueModified: (newVal) => {
            root.valueX = newVal;
            root.valuesChanged(root.valueX, root.valueY, root.valueZ);
        }
    }

    // Y Channel (Green)
    AxisChannel {
        axisLabel: "Y"
        channelColor: "#43a047"
        value: root.valueY
        onValueModified: (newVal) => {
            root.valueY = newVal;
            root.valuesChanged(root.valueX, root.valueY, root.valueZ);
        }
    }

    // Z Channel (Blue)
    AxisChannel {
        axisLabel: "Z"
        channelColor: "#1e88e5"
        value: root.valueZ
        onValueModified: (newVal) => {
            root.valueZ = newVal;
            root.valuesChanged(root.valueX, root.valueY, root.valueZ);
        }
    }
}


