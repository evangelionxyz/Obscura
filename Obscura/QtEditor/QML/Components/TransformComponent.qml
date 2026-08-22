import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Obscura.Editor 1.0

ComponentBase {
    id: root
    title: "Transform"

    property real posX: 0.0
    property real posY: 0.0
    property real posZ: 0.0

    property real rotX: 0.0
    property real rotY: 0.0
    property real rotZ: 0.0

    property real scaleX: 1.0
    property real scaleY: 1.0
    property real scaleZ: 1.0

    signal transformChanged()

    Float3 {
        label: "Location"
        valueX: root.posX
        valueY: root.posY
        valueZ: root.posZ
        onValuesChanged: (x, y, z) => {
            root.posX = x;
            root.posY = y;
            root.posZ = z;
            root.transformChanged();
        }
    }

    Float3 {
        label: "Rotation"
        valueX: root.rotX
        valueY: root.rotY
        valueZ: root.rotZ
        onValuesChanged: (x, y, z) => {
            root.rotX = x;
            root.rotY = y;
            root.rotZ = z;
            root.transformChanged();
        }
    }

    Float3 {
        label: "Scale"
        valueX: root.scaleX
        valueY: root.scaleY
        valueZ: root.scaleZ
        onValuesChanged: (x, y, z) => {
            root.scaleX = x;
            root.scaleY = y;
            root.scaleZ = z;
            root.transformChanged();
        }
    }
}
