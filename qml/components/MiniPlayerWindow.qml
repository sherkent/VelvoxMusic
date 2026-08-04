import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

import VelvoxMusic

/**
 * @brief Standalone always-on-top mini player window.
 *
 * Shows cover + title/artist + compact playback controls. The main window can
 * hide itself and show this window instead, and vice versa.
 */
Window {
    id: root

    width: 320
    height: 120
    minimumWidth: 280
    minimumHeight: 100

    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool
    color: "transparent"
    visible: false

    property var currentTrack: ({
        title: "",
        artist: "",
        coverPath: "",
        filePath: ""
    })

    signal requestRestore()
    signal requestPrevious()
    signal requestNext()

    function togglePlayPause() {
        if (audioEngine.playbackState === AudioEngine.Playing)
            audioEngine.pause()
        else if (audioEngine.playbackState === AudioEngine.Paused)
            audioEngine.resume()
        else if (root.currentTrack.filePath)
            audioEngine.play(root.currentTrack.filePath)
    }

    Component.onCompleted: windowBackdrop.apply(root, WindowBackdrop.BlurBehind)

    // Semi-transparent overlay — the window's BlurBehind provides the real
    // acrylic blur of whatever is behind the window on the desktop.
    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(ThemeManager.panelColor.r, ThemeManager.panelColor.g,
                       ThemeManager.panelColor.b, 0.72)
        radius: ThemeManager.radiusMedium

        MouseArea {
            id: dragArea
            anchors.fill: parent
            onPressed: root.startSystemMove()
            onDoubleClicked: root.requestRestore()
        }

        RowLayout {
            anchors.fill: parent
            anchors.margins: ThemeManager.spacingMedium
            spacing: ThemeManager.spacingMedium

            Rectangle {
                Layout.preferredWidth: 88
                Layout.preferredHeight: 88
                color: ThemeManager.titleBarColor
                radius: ThemeManager.radiusMedium
                clip: true

                CrossFadeImage {
                    anchors.fill: parent
                    source: root.currentTrack.coverPath ? "file:///" + root.currentTrack.coverPath : ""
                    fillMode: Image.PreserveAspectCrop
                    sourceSize: Qt.size(88, 88)
                    visible: source.length > 0
                }

                Text {
                    anchors.centerIn: parent
                    text: "♪"
                    color: ThemeManager.secondaryTextColor
                    font.pixelSize: 32
                    visible: root.currentTrack.coverPath.length === 0
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Text {
                    text: root.currentTrack.title || qsTr("No track")
                    color: ThemeManager.textColor
                    font.pixelSize: ThemeManager.fontSizeLarge
                    font.bold: true
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Text {
                    text: root.currentTrack.artist || ""
                    color: ThemeManager.secondaryTextColor
                    font.pixelSize: ThemeManager.fontSizeMedium
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                RowLayout {
                    spacing: ThemeManager.spacingMedium
                    Layout.alignment: Qt.AlignHCenter

                    Button {
                        text: "⏮"
                        flat: true
                        enabled: root.currentTrack.filePath.length > 0
                        onClicked: root.requestPrevious()
                        contentItem: Text {
                            text: parent.text
                            color: ThemeManager.secondaryTextColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: parent.hovered ? Qt.rgba(1, 1, 1, 0.08) : "transparent"
                            radius: ThemeManager.radiusSmall
                            Behavior on color { ColorAnimation { duration: Motion.durationFast } }
                        }
                    }

                    PlayPauseButton {
                        playing: audioEngine.playbackState === AudioEngine.Playing
                        onClicked: root.togglePlayPause()
                    }

                    Button {
                        text: "⏭"
                        flat: true
                        enabled: root.currentTrack.filePath.length > 0
                        onClicked: root.requestNext()
                        contentItem: Text {
                            text: parent.text
                            color: ThemeManager.secondaryTextColor
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: Rectangle {
                            color: parent.hovered ? Qt.rgba(1, 1, 1, 0.08) : "transparent"
                            radius: ThemeManager.radiusSmall
                            Behavior on color { ColorAnimation { duration: Motion.durationFast } }
                        }
                    }
                }
            }

            Button {
                text: "×"
                flat: true
                Layout.alignment: Qt.AlignTop
                contentItem: Text {
                    text: parent.text
                    color: ThemeManager.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: parent.hovered ? Qt.rgba(1, 1, 1, 0.1) : "transparent"
                    radius: ThemeManager.radiusSmall
                    Behavior on color { ColorAnimation { duration: Motion.durationFast } }
                }
                onClicked: root.requestRestore()
            }
        }
    }
}
