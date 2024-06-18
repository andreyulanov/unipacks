import QtQuick 2.0
import QtQuick.Window 2.14
import QtLocation 5.6
import QtPositioning 5.6

Window {
    width: Screen.width
    height: Screen.height
    visible: true

    Plugin {
        id: mapPlugin
        name: "bingmaps"
    }

    Map {
        anchors.fill: parent
        plugin: mapPlugin
        center: QtPositioning.coordinate(60, 30)
        zoomLevel: 3
    }
}
