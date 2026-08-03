import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

import VelvoxMusic

/**
 * @brief Album browser page.
 *
 * Displays a grid of albums on the left and the selected album's tracks on the
 * right. Clicking a track starts playback via the main window helper.
 */
Page {
    id: root

    background: Rectangle { color: ThemeManager.surfaceColor }

    property int selectedIndex: -1
    property var selectedTracks: []
    property string selectedName: ""

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: ThemeManager.spacingLarge
        spacing: ThemeManager.spacingMedium

        Text {
            text: qsTr("Albums")
            color: ThemeManager.textColor
            font.pixelSize: ThemeManager.fontSizeTitle
            font.bold: true
        }

        Text {
            text: qsTr("%1 albums").arg(albumListModel.count)
            color: ThemeManager.secondaryTextColor
            font.pixelSize: ThemeManager.fontSizeMedium
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: ThemeManager.spacingMedium

            GridView {
                id: albumGrid
                Layout.preferredWidth: root.width * 0.55
                Layout.fillHeight: true
                clip: true
                cellWidth: 160
                cellHeight: 220
                model: albumListModel
                cacheBuffer: Math.max(200, height * 1.5)

                delegate: Rectangle {
                    id: albumDelegate
                    width: albumGrid.cellWidth - ThemeManager.spacingSmall
                    height: albumGrid.cellHeight - ThemeManager.spacingSmall
                    radius: ThemeManager.radiusMedium
                    color: itemMouse.pressed
                            ? Qt.darker(ThemeManager.panelColor, 1.2)
                            : (itemMouse.containsMouse ? ThemeManager.hoverColor : ThemeManager.panelColor)

                    Behavior on color { ColorAnimation { duration: Motion.durationFast } }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: ThemeManager.spacingMedium
                        spacing: ThemeManager.spacingSmall

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: width
                            color: ThemeManager.titleBarColor
                            radius: ThemeManager.radiusSmall
                            clip: true

                            CrossFadeImage {
                                anchors.fill: parent
                                source: coverPath ? "file:///" + coverPath : ""
                                fillMode: Image.PreserveAspectCrop
                                sourceSize: Qt.size(160, 160)
                                animate: false
                                visible: source.length > 0
                            }

                            Text {
                                anchors.centerIn: parent
                                text: "♪"
                                color: ThemeManager.secondaryTextColor
                                font.pixelSize: 32
                                visible: coverPath.length === 0
                            }
                        }

                        Text {
                            text: name || qsTr("Unknown album")
                            color: ThemeManager.textColor
                            font.pixelSize: ThemeManager.fontSizeMedium
                            font.bold: true
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Text {
                            text: (albumArtist || qsTr("Unknown artist")) + " · " + trackCount + " " + qsTr("tracks")
                            color: ThemeManager.secondaryTextColor
                            font.pixelSize: ThemeManager.fontSizeSmall
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    MouseArea {
                        id: itemMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            root.selectedIndex = index
                            root.selectedName = name
                            root.selectedTracks = albumListModel.tracksAt(index)
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: ThemeManager.panelColor
                radius: ThemeManager.radiusMedium
                visible: root.selectedTracks.length > 0

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: ThemeManager.spacingMedium
                    spacing: ThemeManager.spacingMedium

                    Text {
                        text: root.selectedName
                        color: ThemeManager.textColor
                        font.pixelSize: ThemeManager.fontSizeTitle
                        font.bold: true
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    ListView {
                        id: trackList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: ThemeManager.spacingSmall
                        cacheBuffer: Math.max(200, height * 1.5)
                        model: root.selectedTracks

                        delegate: Rectangle {
                            id: trackDelegate
                            width: trackList.width
                            height: 48
                            radius: ThemeManager.radiusSmall
                            color: trackMouse.pressed
                                    ? Qt.darker(ThemeManager.panelColor, 1.2)
                                    : (trackMouse.containsMouse ? ThemeManager.hoverColor : ThemeManager.surfaceColor)

                            Behavior on color { ColorAnimation { duration: Motion.durationFast } }

                            MouseArea {
                                id: trackMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: Window.window.playTrackFromData(modelData)
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: ThemeManager.spacingMedium
                                spacing: ThemeManager.spacingMedium

                                Text {
                                    text: (index + 1) + "."
                                    color: ThemeManager.secondaryTextColor
                                    font.pixelSize: ThemeManager.fontSizeMedium
                                    Layout.preferredWidth: 24
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2

                                    Text {
                                        text: modelData.title || qsTr("Unknown title")
                                        color: ThemeManager.textColor
                                        font.pixelSize: ThemeManager.fontSizeMedium
                                        font.bold: true
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }

                                    Text {
                                        text: modelData.artist || qsTr("Unknown artist")
                                        color: ThemeManager.secondaryTextColor
                                        font.pixelSize: ThemeManager.fontSizeSmall
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }
                                }

                                Text {
                                    text: modelData.durationText || "0:00"
                                    color: ThemeManager.secondaryTextColor
                                    font.pixelSize: ThemeManager.fontSizeMedium
                                }
                            }
                        }

                        ScrollBar.vertical: ScrollBar {}
                    }
                }
            }
        }
    }
}
