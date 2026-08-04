import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import VelvoxMusic

/**
 * @brief Synchronized scrolling lyrics view.
 *
 * Displays lyric lines from the lyricsModel and auto-scrolls so the currently
 * active line stays centered. The active line is highlighted; inactive lines
 * are dimmed.
 */
Rectangle {
    id: root

    color: Qt.rgba(ThemeManager.surfaceColor.r, ThemeManager.surfaceColor.g, ThemeManager.surfaceColor.b, 0.88)
    radius: ThemeManager.radiusMedium

    ListView {
        id: lyricsList
        anchors.fill: parent
        anchors.margins: ThemeManager.spacingMedium
        clip: true
        spacing: ThemeManager.spacingMedium
        model: lyricsModel
        cacheBuffer: Math.max(200, height * 1.5)

        // Keep the active line vertically centered.
        highlightRangeMode: ListView.ApplyRange
        preferredHighlightBegin: height / 2 - 24
        preferredHighlightEnd: height / 2 + 24
        currentIndex: lyricsModel.currentIndex

        delegate: Row {
            id: lineRow
            width: lyricsList.width
            spacing: 4

            // Expose delegate model data to nested Repeater children.
            property bool lineActive: model.active
            property int lineWordIndex: model.wordIndex

            // Center the line by computing its implicit width.
            x: (lyricsList.width - implicitWidth) / 2

            Repeater {
                model: model.words

                Text {
                    id: wordItem
                    text: modelData.text
                    color: {
                        if (!lineRow.lineActive)
                            return ThemeManager.secondaryTextColor
                        return (index <= lineRow.lineWordIndex)
                                ? ThemeManager.textColor
                                : ThemeManager.secondaryTextColor
                    }
                    font.pixelSize: lineRow.lineActive ? ThemeManager.fontSizeLarge : ThemeManager.fontSizeMedium
                    font.bold: lineRow.lineActive

                    Behavior on color { ColorAnimation { duration: Motion.durationFast } }
                    Behavior on font.pixelSize { NumberAnimation { duration: Motion.durationFast; easing: Motion.easeOutCubic } }
                }
            }
        }

        // Smooth scroll when the active index changes.
        Behavior on contentY {
            enabled: !Motion.reduceMotion
            NumberAnimation { duration: Motion.durationNormal; easing: Motion.easeOutCubic }
        }
    }

    Text {
        anchors.centerIn: parent
        text: qsTr("No lyrics available")
        color: ThemeManager.secondaryTextColor
        font.pixelSize: ThemeManager.fontSizeLarge
        visible: !lyricsModel.hasLyrics
    }
}
