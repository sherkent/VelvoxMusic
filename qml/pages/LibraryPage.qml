import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

import VelvoxMusic

Page {
    id: root

    background: Rectangle { color: ThemeManager.surfaceColor }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: ThemeManager.spacingLarge
        spacing: ThemeManager.spacingMedium

        Text {
            text: qsTr("Music Library")
            color: ThemeManager.textColor
            font.pixelSize: ThemeManager.fontSizeTitle
            font.bold: true
        }

        RowLayout {
            spacing: ThemeManager.spacingMedium
            Layout.fillWidth: true

            TextField {
                id: dirField
                Layout.fillWidth: true
                placeholderText: qsTr("C:/Music")
                text: "C:/Music"
                color: ThemeManager.textColor
            }

            AppButton {
                text: qsTr("Add")
                onClicked: libraryManager.addMusicDirectory(dirField.text)
            }

            AppButton {
                text: qsTr("Scan")
                enabled: !libraryManager.scanning
                onClicked: libraryManager.startScan()
            }

            Text {
                text: libraryManager.scanning
                      ? qsTr("Scanning...")
                      : qsTr("Ready")
                color: ThemeManager.secondaryTextColor
            }
        }

        RowLayout {
            spacing: ThemeManager.spacingMedium
            Layout.fillWidth: true

            TextField {
                id: filterField
                Layout.fillWidth: true
                placeholderText: qsTr("Search title / artist / album...")
                color: ThemeManager.textColor
                onTextChanged: filterTimer.restart()

                Timer {
                    id: filterTimer
                    interval: 150
                    onTriggered: trackListModel.setFilter(filterField.text)
                }
            }

            ComboBox {
                id: sortRoleBox
                model: ListModel {
                    ListElement { text: "Title"; value: "title" }
                    ListElement { text: "Artist"; value: "artist" }
                    ListElement { text: "Album"; value: "album" }
                    ListElement { text: "Date added"; value: "date_added" }
                    ListElement { text: "Duration"; value: "duration" }
                }
                textRole: "text"
                valueRole: "value"
                currentIndex: 3
                onActivated: trackListModel.setSortRole(sortRoleBox.currentValue)
            }

            ComboBox {
                id: sortOrderBox
                model: [qsTr("Ascending"), qsTr("Descending")]
                currentIndex: 1
                onActivated: trackListModel.setSortOrder(currentIndex === 0 ? Qt.AscendingOrder : Qt.DescendingOrder)
            }
        }

        Text {
            text: qsTr("%1 tracks").arg(trackListModel.count)
            color: ThemeManager.secondaryTextColor
            font.pixelSize: ThemeManager.fontSizeMedium
        }

        ListView {
            id: trackList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: trackListModel
            spacing: ThemeManager.spacingSmall
            cacheBuffer: Math.max(200, height * 1.5)

            delegate: Rectangle {
                id: itemDelegate
                width: trackList.width
                height: 64
                radius: ThemeManager.radiusMedium

                // Staggered appear-on-scroll animation. Each delegate instance
                // animates only once, so fast scrolling / reuse does not retrigger.
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
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onClicked: {
                        if (mouse.button === Qt.RightButton)
                            contextMenu.popup()
                        else
                            Window.window.playTrack(index)
                    }

                    Menu {
                        id: contextMenu

                        MenuItem {
                            text: qsTr("Play")
                            onTriggered: Window.window.playTrack(index)
                        }
                        MenuItem {
                            text: qsTr("Add to playlist")
                            onTriggered: addToPlaylistMenu.popup()
                        }
                    }

                    Menu {
                        id: addToPlaylistMenu

                        Instantiator {
                            model: playlistModel.count
                            delegate: MenuItem {
                                text: playlistModel.data(playlistModel.index(index, 0), PlaylistModel.NameRole)
                                onTriggered: {
                                    let trackId = trackListModel.trackIdAt(index)
                                    let playlistId = playlistModel.data(playlistModel.index(index, 0), PlaylistModel.IdRole)
                                    playlistModel.addTrack(playlistId, trackId)
                                }
                            }
                            onObjectAdded: (index, object) => addToPlaylistMenu.insertItem(index, object)
                            onObjectRemoved: (index, object) => addToPlaylistMenu.removeItem(object)
                        }
                    }
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
                            source: coverPath ? "file:///" + coverPath : ""
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
                            visible: coverPath.length === 0
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Text {
                            text: title || qsTr("Unknown title")
                            color: ThemeManager.textColor
                            font.pixelSize: ThemeManager.fontSizeLarge
                            font.bold: true
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Text {
                            text: (artist || qsTr("Unknown artist")) + " - " + (album || qsTr("Unknown album"))
                            color: ThemeManager.secondaryTextColor
                            font.pixelSize: ThemeManager.fontSizeMedium
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    Text {
                        text: durationText
                        color: ThemeManager.secondaryTextColor
                        Layout.preferredWidth: 50
                    }
                }
            }

            ScrollBar.vertical: ScrollBar {}
        }
    }
}
