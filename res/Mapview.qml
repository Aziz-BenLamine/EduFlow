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
        Behavior on zoomLevel {
            NumberAnimation { duration: 300; easing.type: Easing.InOutQuad }
        }
        Behavior on center {
            CoordinateAnimation { duration: 300; easing.type: Easing.InOutQuad }
        }
        plugin: Plugin {
            name: "osm"
            PluginParameter { name: "osm.mapping.providersavailable"; value: "true" }
            PluginParameter { name: "osm.mapping.cache.directory"; value: "cache" }
            PluginParameter { name: "osm.mapping.highdpi_tiles"; value: "true" }
        }

        Button {
            id: zoomIn
            text: "+"
            width: 40
            height: 40
            anchors {
                right: parent.right
                rightMargin: 10
                top: parent.top
                topMargin: 10
            }
            background: Rectangle {
                color: "#4CAF50"
                radius: 5
                opacity: zoomIn.pressed ? 0.7 : 1.0
            }
            contentItem: Text {
                text: zoomIn.text
                color: "white"
                font.bold: true
                font.pixelSize: 20
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            onClicked: {
                if (map.zoomLevel < map.maximumZoomLevel) {
                    map.zoomLevel = map.zoomLevel + 1
                }
            }
        }

        Button {
            id: zoomOut
            text: "-"
            width: 40
            height: 40
            anchors {
                right: parent.right
                rightMargin: 10
                top: zoomIn.bottom
                topMargin: 5
            }
            background: Rectangle {
                color: "#F44336"
                radius: 5
                opacity: zoomOut.pressed ? 0.7 : 1.0
            }
            contentItem: Text {
                text: zoomOut.text
                color: "white"
                font.bold: true
                font.pixelSize: 20
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            onClicked: {
                if (map.zoomLevel > map.minimumZoomLevel) {
                    map.zoomLevel = map.zoomLevel - 1
                }
            }
        }

        MouseArea {
            anchors.fill: parent
            property real lastX: 0
            property real lastY: 0

            // Change le curseur pour indiquer que la carte peut être déplacée
            cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor

            function handlePress(mouse) {
                lastX = mouse.x
                lastY = mouse.y
            }

            function handleMove(mouse) {
                if (pressed) {
                    var dx = mouse.x - lastX
                    var dy = mouse.y - lastY
                    map.pan(-dx, -dy) // Déplace la carte dans la direction opposée au mouvement

                    // Vérifie les limites géographiques après le déplacement
                    var newLat = map.center.latitude
                    var newLon = map.center.longitude

                    // Limite la latitude entre -90 et 90 degrés
                    if (newLat > 90) {
                        map.center = QtPositioning.coordinate(90, newLon)
                    } else if (newLat < -90) {
                        map.center = QtPositioning.coordinate(-90, newLon)
                    }

                    // Limite la longitude entre -180 et 180 degrés
                    if (newLon > 180) {
                        map.center = QtPositioning.coordinate(newLat, 180)
                    } else if (newLon < -180) {
                        map.center = QtPositioning.coordinate(newLat, -180)
                    }

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
