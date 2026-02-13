import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import GodotQmlBridge

ApplicationWindow {
    id: root
    width: 1280
    height: 720
    visible: true
    title: "PV Simulation Platform"

    // C++ backend, registered as QML_ELEMENT
    GodotBridge {
        id: godotBridge

        onMessageReceived: (event, data) => {
            if (event === "object_selected") {
                console.log("Selected:", data.id)
            }
        }

        onErrorOccurred: (error) => {
            errorLabel.text = error
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Left panel -- controls
        Rectangle {
            Layout.preferredWidth: 300
            Layout.fillHeight: true
            color: "#2b2b2b"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10

                Label {
                    text: "Controls"
                    color: "white"
                    font.pixelSize: 18
                }

                Button {
                    text: godotBridge.connected ? "Connected" : "Launch Godot"
                    enabled: !godotBridge.connected
                    onClicked: godotBridge.launch("C:/path/to/godot/project")
                }

                Button {
                    text: "Move Satellite"
                    enabled: godotBridge.connected
                    onClicked: {
                        godotBridge.sendCommand("set_satellite_position", {
                            "id": "sat1",
                            "pos": [100, 200, 300]
                        })
                    }
                }

                // Status indicator
                Label {
                    id: errorLabel
                    color: "red"
                    wrapMode: Text.Wrap
                }

                Item { Layout.fillHeight: true }  // Spacer
            }
        }

        // Right side -- embedded Godot viewport
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // Placeholder until Godot is ready
            Rectangle {
                anchors.fill: parent
                color: "#1a1a1a"
                visible: !godotBridge.godotWindow

                Label {
                    anchors.centerIn: parent
                    text: "3D Viewport — Launch Godot to begin"
                    color: "#666"
                    font.pixelSize: 16
                }
            }

            // Embedded Godot window (Qt 6.7+)
            WindowContainer {
                id: godotContainer
                anchors.fill: parent
                window: godotBridge.godotWindow
                visible: godotBridge.godotWindow !== null

                onWidthChanged: {
                    if (godotBridge.connected) {
                        godotBridge.sendCommand("resize_viewport", {
                            "width": width,
                            "height": height
                        })
                    }
                }
                onHeightChanged: {
                    if (godotBridge.connected) {
                        godotBridge.sendCommand("resize_viewport", {
                            "width": width,
                            "height": height
                        })
                    }
                }
            }
        }
    }
}
