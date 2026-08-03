import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

import VelvoxMusic

/**
 * @brief Compact floating mini player wrapped in an acrylic/glass panel.
 *
 * The caller controls visibility and passes the blurred background source and
 * the current render quality.
 */
Item {
    id: root

    property Item source: null
    property int renderQuality: AcrylicPanel.Quality.Medium

    width: 320
    height: 120

    AcrylicPanel {
        anchors.fill: parent
        source: root.source
        blurRadius: 48
        tintColor: ThemeManager.panelColor
        tintOpacity: 0.78
        renderQuality: root.renderQuality

        RowLayout {
            anchors.fill: parent
            anchors.margins: ThemeManager.spacingMedium
            spacing: ThemeManager.spacingMedium

            Rectangle {
                Layout.preferredWidth: 88
                Layout.preferredHeight: 88
                color: ThemeManager.titleBarColor
                radius: ThemeManager.radiusMedium

                CrossFadeImage {
                    anchors.fill: parent
                    source: Window.window
                              && Window.window.currentTrack.coverPath
                                ? "file:///" + Window.window.currentTrack.coverPath
                                : ""
                    fillMode: Image.PreserveAspectCrop
                    sourceSize: Qt.size(88, 88)
                    visible: source.length > 0
                }

                Text {
                    anchors.centerIn: parent
                    text: "♪"
                    color: ThemeManager.secondaryTextColor
                    font.pixelSize: 32
                    visible: !Window.window
                             || Window.window.currentTrack.coverPath.length === 0
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Text {
                    text: Window.window
                          ? (Window.window.currentTrack.title || qsTr("No track"))
                          : qsTr("No track")
                    color: ThemeManager.textColor
                    font.pixelSize: ThemeManager.fontSizeLarge
                    font.bold: true
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Text {
                    text: Window.window
                          ? (Window.window.currentTrack.artist || "")
                          : ""
                    color: ThemeManager.secondaryTextColor
                    font.pixelSize: ThemeManager.fontSizeMedium
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                RowLayout {
                    spacing: ThemeManager.spacingMedium
                    Layout.alignment: Qt.AlignHCenter

                    PlayPauseButton {
                        playing: Window.window && Window.window.audioEngine.playbackState === AudioEngine.Playing
                        onClicked: {
                            if (!Window.window)
                                return
                            const engine = Window.window.audioEngine
                            const track = Window.window.currentTrack
                            if (engine.playbackState === AudioEngine.Playing)
                                engine.pause()
                            else if (engine.playbackState === AudioEngine.Paused)
                                engine.resume()
                            else if (track.filePath)
                                engine.play(track.filePath)
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
                background: Rectangle { color: "transparent" }
                onClicked: {
                    if (Window.window)
                        Window.window.miniPlayerVisible = false
                }
            }
        }
    }
}
