import QtQuick 2.15
import QtQuick.Controls 2.15
import QtLocation 5.15
import QtPositioning 5.15

ApplicationWindow {
    id: root
    visible: true
    width: 800
    height: 600
    title: "Carte de la Tunisie"

    Map {
        id: map
        anchors.fill: parent
        center: QtPositioning.coordinate(36.8065, 10.1815)
        zoomLevel: 10
        plugin: Plugin {
            name: "osm"
            PluginParameter { name: "osm.mapping.providersavailable"; value: "true" }
            PluginParameter { name: "osm.mapping.cache.directory"; value: "cache" }
            PluginParameter { name: "osm.mapping.highdpi_tiles"; value: "true" }
            PluginParameter { name: "osm.mapping.host"; value: "https://tile.openstreetmap.org"}
        }

        // Bouton Zoom In
        Button {
            id: zoomIn
            text: "+"
            width: 48
            height: 48
            anchors {
                right: parent.right
                rightMargin: 15
                top: parent.top
                topMargin: 15
            }

            z: 1 // bouton est au-dessus du MouseArea

            background: Rectangle {
                radius: 24
                color: "#4CAF50"
                border.color: Qt.lighter("#4CAF50", 1.2)
                border.width: 1
                Rectangle {
                    anchors.fill: parent
                    radius: 24
                    color: "black"
                    opacity: 0.2
                    z: -1
                    anchors.margins: -4
                    transform: Translate { y: 2 }
                }
                scale: zoomIn.pressed ? 0.95 : 1.0
                Behavior on scale { NumberAnimation { duration: 100 } }
            }
            contentItem: Text {
                text: zoomIn.text
                color: "white"
                font.pixelSize: 24
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            onClicked: {
                console.log("Zoom In clicked, current zoom:", map.zoomLevel)
                if (map.zoomLevel < 18) { // Valeur typique pour OSM, ajustable
                    map.zoomLevel += 1
                }
            }
        }

        // Bouton Zoom Out
        Button {
            id: zoomOut
            text: "-"
            width: 48
            height: 48
            anchors {
                right: parent.right
                rightMargin: 15
                top: zoomIn.bottom
                topMargin: 10
            }
            z: 1 // S'assurer que le bouton est au-dessus du MouseArea
            background: Rectangle {
                radius: 24
                color: "#F44336"
                border.color: Qt.lighter("#F44336", 1.2)
                border.width: 1
                Rectangle {
                    anchors.fill: parent
                    radius: 24
                    color: "black"
                    opacity: 0.2
                    z: -1
                    anchors.margins: -4
                    transform: Translate { y: 2 }
                }
                scale: zoomOut.pressed ? 0.95 : 1.0
                Behavior on scale { NumberAnimation { duration: 100 } }
            }
            contentItem: Text {
                text: zoomOut.text
                color: "white"
                font.pixelSize: 24
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            onClicked: {
                console.log("Zoom Out clicked, current zoom:", map.zoomLevel)
                if (map.zoomLevel > 1) { // Valeur typique pour OSM, ajustable
                    map.zoomLevel -= 1
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            property real lastX: 0
            property real lastY: 0
            cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor

            function handlePress(mouse) {
                lastX = mouse.x
                lastY = mouse.y
            }

            function handleMove(mouse) {
                if (pressed) {
                    var dx = mouse.x - lastX
                    var dy = mouse.y - lastY
                    map.pan(-dx, -dy)
                    lastX = mouse.x
                    lastY = mouse.y
                }
            }

            onPressed: handlePress(mouse)
            onPositionChanged: handleMove(mouse)
        }
    }

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 30
        color: "#80000000"
        Text {
            anchors.centerIn: parent
            color: "white"
            text: "Zoom : " + map.zoomLevel.toFixed(1) +
                  " | Lat : " + map.center.latitude.toFixed(4) +
                  " | Lon : " + map.center.longitude.toFixed(4)
        }
    }
}
