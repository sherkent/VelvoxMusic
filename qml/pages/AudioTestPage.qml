import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import VelvoxMusic

Page {
    id: root

    background: Rectangle { color: ThemeManager.backgroundColor }

    property var spectrumData: []

    Connections {
        target: audioEngine
        function onSpectrumChanged(bands) {
            root.spectrumData = bands
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 40
        spacing: 24

        Text {
            text: qsTr("Audio Engine Test")
            color: ThemeManager.textColor
            font.pixelSize: 28
            font.bold: true
        }

        TextField {
            id: pathField
            Layout.fillWidth: true
            text: "C:/Music/test.mp3"
            placeholderText: qsTr("Absolute path to mp3/flac/wav/ogg...")
            color: ThemeManager.textColor
        }

        RowLayout {
            spacing: 12

            AppButton {
                text: {
                    if (audioEngine.playbackState === AudioEngine.Playing)
                        return qsTr("Pause")
                    if (audioEngine.playbackState === AudioEngine.Paused)
                        return qsTr("Resume")
                    return qsTr("Play")
                }
                onClicked: {
                    if (audioEngine.playbackState === AudioEngine.Playing)
                        audioEngine.pause()
                    else if (audioEngine.playbackState === AudioEngine.Paused)
                        audioEngine.resume()
                    else
                        audioEngine.play(pathField.text)
                }
            }

            AppButton {
                text: qsTr("Stop")
                onClicked: audioEngine.stop()
            }
        }

        RowLayout {
            spacing: 12

            Text {
                text: formatTime(audioEngine.position)
                color: ThemeManager.textColor
                Layout.preferredWidth: 50
            }

            Slider {
                id: positionSlider
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
                text: formatTime(audioEngine.duration)
                color: ThemeManager.textColor
                Layout.preferredWidth: 50
            }
        }

        RowLayout {
            spacing: 12

            Text {
                text: qsTr("Volume")
                color: ThemeManager.textColor
            }

            Slider {
                Layout.fillWidth: true
                from: 0
                to: 1
                value: audioEngine.volume
                onMoved: audioEngine.volume = value
            }
        }

        Text {
            text: qsTr("State: %1").arg(stateName(audioEngine.playbackState))
            color: ThemeManager.secondaryTextColor
            font.pixelSize: 14
        }

        // Spectrum visualization: 32 vertical bars drawn in a single Canvas
        // to avoid re-laying out 32 Rectangles inside RowLayout on every frame.
        Canvas {
            id: spectrumCanvas
            Layout.fillWidth: true
            Layout.fillHeight: true

            property var bands: root.spectrumData
            onBandsChanged: requestPaint()

            onPaint: {
                const ctx = getContext("2d")
                const w = width
                const h = height
                const count = 32
                const gap = 4
                const barW = (w - gap * (count - 1)) / count
                ctx.clearRect(0, 0, w, h)
                ctx.fillStyle = ThemeManager.accentColor

                for (let i = 0; i < count; ++i) {
                    const value = (i < bands.length) ? bands[i] : 0
                    const barH = h * value
                    const x = i * (barW + gap)
                    const y = h - barH
                    ctx.fillRect(x, y, barW, barH)
                }
            }
        }
    }

    function formatTime(ms) {
        if (!ms || ms < 0)
            return "0:00"
        const totalSeconds = Math.floor(ms / 1000)
        const minutes = Math.floor(totalSeconds / 60)
        const seconds = totalSeconds % 60
        return minutes + ":" + (seconds < 10 ? "0" + seconds : seconds)
    }

    function stateName(state) {
        switch (state) {
        case AudioEngine.Playing: return "Playing"
        case AudioEngine.Paused: return "Paused"
        case AudioEngine.Stopped: return "Stopped"
        default: return "Unknown"
        }
    }
}
