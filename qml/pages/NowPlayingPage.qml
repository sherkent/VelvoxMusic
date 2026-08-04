import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects

import VelvoxMusic

/**
 * @brief Full-screen now-playing view with a cover-driven dynamic gradient.
 *
 * The background reuses the mesh gradient shader from the earlier phase but is
 * fed a palette extracted from the current track's cover art. The shader only
 * animates while this page is visible.
 */
Rectangle {
    id: page

    color: "transparent"

    property var currentTrack: rootWindow.currentTrack
    property var paletteColors: []
    property bool pageActive: false

    function requestPalette() {
        if (!pageActive || !currentTrack || currentTrack.id < 0)
            return
        coverColorExtractor.extract(currentTrack.id, currentTrack.coverPath || "")
    }

    onCurrentTrackChanged: requestPalette()
    onPageActiveChanged: requestPalette()

    Connections {
        target: coverColorExtractor
        function onExtractionFinished(trackId, colors) {
            if (trackId === currentTrack.id)
                page.paletteColors = colors || []
        }
    }

    // Background: mesh gradient driven by the extracted cover palette.
    MeshGradientBackground {
        id: background
        anchors.fill: parent
        colors: page.paletteColors
        running: page.pageActive && background.windowActive
        renderQuality: MeshGradientBackground.Medium
        z: -1
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: ThemeManager.spacingLarge
        spacing: ThemeManager.spacingLarge

        // Top bar with back button
        RowLayout {
            Layout.fillWidth: true
            spacing: ThemeManager.spacingMedium

            Button {
                text: qsTr("Back")
                flat: true
                onClicked: stackView.pop()
                contentItem: Text {
                    text: parent.text
                    color: ThemeManager.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: parent.hovered ? Qt.rgba(1, 1, 1, 0.08) : "transparent"
                    radius: ThemeManager.radiusSmall
                    Behavior on color { ColorAnimation { duration: Motion.durationFast } }
                }
            }

            Item { Layout.fillWidth: true }

            Text {
                text: qsTr("Now Playing")
                color: ThemeManager.textColor
                font.pixelSize: ThemeManager.fontSizeLarge
                font.bold: true
            }

            Item { Layout.fillWidth: true }
        }

        Item { Layout.fillHeight: true }

        // Large cover
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 320
            Layout.preferredHeight: 320
            color: ThemeManager.panelColor
            radius: ThemeManager.radiusLarge
            clip: true

            CrossFadeImage {
                anchors.fill: parent
                source: currentTrack.coverPath ? "file:///" + currentTrack.coverPath : ""
                fillMode: Image.PreserveAspectCrop
                sourceSize: Qt.size(640, 640)
                visible: source.length > 0
            }

            Text {
                anchors.centerIn: parent
                text: "♪"
                color: ThemeManager.secondaryTextColor
                font.pixelSize: 96
                visible: currentTrack.coverPath.length === 0
            }
        }

        // Track info
        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: ThemeManager.spacingSmall

            Text {
                text: currentTrack.title || qsTr("No track selected")
                color: ThemeManager.textColor
                font.pixelSize: ThemeManager.fontSizeTitle
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                Layout.alignment: Qt.AlignHCenter
            }

            Text {
                text: currentTrack.artist || ""
                color: ThemeManager.secondaryTextColor
                font.pixelSize: ThemeManager.fontSizeLarge
                horizontalAlignment: Text.AlignHCenter
                Layout.alignment: Qt.AlignHCenter
            }
        }

        // Progress
        RowLayout {
            Layout.fillWidth: true
            Layout.maximumWidth: 480
            Layout.alignment: Qt.AlignHCenter
            spacing: ThemeManager.spacingSmall

            Text {
                text: rootWindow.formatTime(audioEngine.position)
                color: ThemeManager.secondaryTextColor
                Layout.preferredWidth: 45
            }

            Slider {
                Layout.fillWidth: true
                from: 0
                to: Math.max(audioEngine.duration, 1)
                value: audioEngine.position
                onPressedChanged: {
                    if (!pressed)
                        audioEngine.seek(value)
                }
            }

            Text {
                text: rootWindow.formatTime(audioEngine.duration)
                color: ThemeManager.secondaryTextColor
                Layout.preferredWidth: 45
            }
        }

        // Playback controls
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: ThemeManager.spacingLarge

            Button {
                text: "⏮"
                flat: true
                enabled: rootWindow.currentTrackSourceRow >= 0
                onClicked: rootWindow.playPrevious()
                contentItem: Text {
                    text: parent.text
                    color: ThemeManager.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 24
                }
                background: Rectangle {
                    color: parent.hovered ? Qt.rgba(1, 1, 1, 0.08) : "transparent"
                    radius: ThemeManager.radiusSmall
                    Behavior on color { ColorAnimation { duration: Motion.durationFast } }
                }
            }

            PlayPauseButton {
                playing: audioEngine.playbackState === AudioEngine.Playing
                Layout.preferredWidth: 72
                Layout.preferredHeight: 72
                onClicked: rootWindow.togglePlayPause()
            }

            Button {
                text: "⏭"
                flat: true
                enabled: rootWindow.currentTrackSourceRow >= 0
                onClicked: rootWindow.playNext()
                contentItem: Text {
                    text: parent.text
                    color: ThemeManager.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font.pixelSize: 24
                }
                background: Rectangle {
                    color: parent.hovered ? Qt.rgba(1, 1, 1, 0.08) : "transparent"
                    radius: ThemeManager.radiusSmall
                    Behavior on color { ColorAnimation { duration: Motion.durationFast } }
                }
            }
        }

        Item { Layout.fillHeight: true }
    }

    StackView.onActivated: page.pageActive = true
    StackView.onDeactivated: page.pageActive = false
}
