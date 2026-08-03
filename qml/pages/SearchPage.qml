import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

import VelvoxMusic

/**
 * @brief Global search page.
 *
 * Searches title/artist/album with fuzzy matching. Filtering is debounced so
 * typing fast does not trigger a model reset on every keystroke.
 */
Page {
    id: root

    background: Rectangle { color: ThemeManager.surfaceColor }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: ThemeManager.spacingLarge
        spacing: ThemeManager.spacingMedium

        Text {
            text: qsTr("Search")
            color: ThemeManager.textColor
            font.pixelSize: ThemeManager.fontSizeTitle
            font.bold: true
        }

        TextField {
            id: searchField
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            placeholderText: qsTr("Search songs, artists, albums...")
            color: ThemeManager.textColor
            font.pixelSize: ThemeManager.fontSizeLarge

            background: Rectangle {
                color: ThemeManager.panelColor
                radius: ThemeManager.radiusMedium
                border.color: searchField.activeFocus ? ThemeManager.accentColor : "transparent"
                border.width: 2

                Behavior on border.color { ColorAnimation { duration: Motion.durationFast } }
            }

            onTextChanged: searchTimer.restart()

            Timer {
                id: searchTimer
                interval: 150
                onTriggered: trackListModel.setFilter(searchField.text)
            }
        }

        Text {
            text: qsTr("%1 results").arg(trackListModel.count)
            color: ThemeManager.secondaryTextColor
            font.pixelSize: ThemeManager.fontSizeMedium
        }

        ListView {
            id: resultList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: ThemeManager.spacingSmall
            model: trackListModel
            cacheBuffer: Math.max(200, height * 1.5)

            delegate: Rectangle {
                id: itemDelegate
                width: resultList.width
                height: 64
                radius: ThemeManager.radiusMedium

                property bool appeared: false
                opacity: appeared ? 1 : 0
                x: appeared ? 0 : -20

                Component.onCompleted: {
                    if (Motion.reduceMotion)
                        appeared = true
                    else
                        appearTimer.start()
                }

                Timer {
                    id: appearTimer
                    interval: (index % 5) * 40
                    onTriggered: itemDelegate.appeared = true
                }

                Behavior on opacity { NumberAnimation { duration: Motion.durationNormal; easing: Motion.easeOutCubic } }
                Behavior on x { NumberAnimation { duration: Motion.durationNormal; easing: Motion.easeOutCubic } }
                Behavior on color { ColorAnimation { duration: Motion.durationFast } }

                color: itemMouse.pressed
                        ? Qt.darker(ThemeManager.panelColor, 1.2)
                        : (itemMouse.containsMouse ? ThemeManager.hoverColor : ThemeManager.panelColor)

                MouseArea {
                    id: itemMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: Window.window.playTrack(index)
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: ThemeManager.spacingMedium
                    spacing: ThemeManager.spacingMedium

                    Rectangle {
                        Layout.preferredWidth: 48
                        Layout.preferredHeight: 48
                        color: ThemeManager.titleBarColor
                        radius: ThemeManager.radiusSmall

                        CrossFadeImage {
                            anchors.fill: parent
                            source: model.coverPath ? "file:///" + model.coverPath : ""
                            fillMode: Image.PreserveAspectCrop
                            sourceSize: Qt.size(48, 48)
                            animate: false
                            visible: source.length > 0
                        }

                        Text {
                            anchors.centerIn: parent
                            text: "♪"
                            color: ThemeManager.secondaryTextColor
                            font.pixelSize: 20
                            visible: model.coverPath.length === 0
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Text {
                            text: model.title || qsTr("Unknown title")
                            color: ThemeManager.textColor
                            font.pixelSize: ThemeManager.fontSizeLarge
                            font.bold: true
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Text {
                            text: (model.artist || qsTr("Unknown artist")) + " · " + (model.album || "")
                            color: ThemeManager.secondaryTextColor
                            font.pixelSize: ThemeManager.fontSizeMedium
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    Text {
                        text: model.durationText
                        color: ThemeManager.secondaryTextColor
                        font.pixelSize: ThemeManager.fontSizeMedium
                    }
                }
            }
        }
    }
}
