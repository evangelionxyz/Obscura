import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Obscura.Editor 1.0

ComponentBase {
    id: root
    title: "Transform"
    removable: false

    property real posX: 0.0
    property real posY: 0.0
    property real posZ: 0.0

    property real rotX: 0.0
    property real rotY: 0.0
    property real rotZ: 0.0

    property real scaleX: 1.0
    property real scaleY: 1.0
    property real scaleZ: 1.0

    property bool updating: false

    signal transformChanged()

    function loadFromEntity(entity) {
        if (!entity) return;
        root.updating = true;
        root.posX = (entity.posX !== undefined) ? Number(entity.posX) : 0.0;
        root.posY = (entity.posY !== undefined) ? Number(entity.posY) : 0.0;
        root.posZ = (entity.posZ !== undefined) ? Number(entity.posZ) : 0.0;
        root.rotX = (entity.rotX !== undefined) ? Number(entity.rotX) : 0.0;
        root.rotY = (entity.rotY !== undefined) ? Number(entity.rotY) : 0.0;
        root.rotZ = (entity.rotZ !== undefined) ? Number(entity.rotZ) : 0.0;
        root.scaleX = (entity.scaleX !== undefined) ? Number(entity.scaleX) : 1.0;
        root.scaleY = (entity.scaleY !== undefined) ? Number(entity.scaleY) : 1.0;
        root.scaleZ = (entity.scaleZ !== undefined) ? Number(entity.scaleZ) : 1.0;

        locFloat3.setValues(root.posX, root.posY, root.posZ);
        rotFloat3.setValues(root.rotX, root.rotY, root.rotZ);
        scaleFloat3.setValues(root.scaleX, root.scaleY, root.scaleZ);

        root.updating = false;
    }

    Float3 {
        id: locFloat3
        label: "Location"
        valueX: root.posX
        valueY: root.posY
        valueZ: root.posZ
        onValuesChanged: (x, y, z) => {
            if (root.updating) return;
            root.posX = x;
            root.posY = y;
            root.posZ = z;
            root.transformChanged();
        }
    }

    Float3 {
        id: rotFloat3
        label: "Rotation"
        valueX: root.rotX
        valueY: root.rotY
        valueZ: root.rotZ
        onValuesChanged: (x, y, z) => {
            if (root.updating) return;
            root.rotX = x;
            root.rotY = y;
            root.rotZ = z;
            root.transformChanged();
        }
    }

    Float3 {
        id: scaleFloat3
        label: "Scale"
        valueX: root.scaleX
        valueY: root.scaleY
        valueZ: root.scaleZ
        onValuesChanged: (x, y, z) => {
            if (root.updating) return;
            root.scaleX = x;
            root.scaleY = y;
            root.scaleZ = z;
            root.transformChanged();
        }
    }
}
