import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

import VelvoxMusic

/**
 * @brief Playlist management page.
 *
 * Supports:
 * - Create / rename / delete playlists
 * - View tracks inside a playlist
 * - Drag tracks to reorder inside a playlist
 * - Right-click a library track to add to a playlist (context menu)
 */
Page {
    id: root

    background: Rectangle { color: ThemeManager.surfaceColor }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: ThemeManager.spacingLarge
        spacing: ThemeManager.spacingMedium

        RowLayout {
            spacing: ThemeManager.spacingMedium

            Text {
                text: playlistModel.showingTracks ? qsTr("Playlist") : qsTr("Playlists")
                color: ThemeManager.textColor
                font.pixelSize: ThemeManager.fontSizeTitle
                font.bold: true
            }

            Item { Layout.fillWidth: true }

            AppButton {
                text: qsTr("New playlist")
                visible: !playlistModel.showingTracks
                onClicked: {
                    const name = "Playlist " + (playlistModel.count + 1)
                    playlistModel.createPlaylist(name)
                }
            }

            AppButton {
                text: qsTr("Back")
                visible: playlistModel.showingTracks
                onClicked: playlistModel.backToPlaylists()
            }
        }

        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: ThemeManager.spacingSmall
            model: playlistModel
            cacheBuffer: Math.max(200, height * 1.5)

            // Reorder inside a playlist.
            moveDisplaced: Transition {
                NumberAnimation { properties: "x,y"; duration: Motion.durationNormal; easing: Motion.easeOutCubic }
            }

            delegate: Rectangle {
                id: itemDelegate
                width: listView.width
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
                    property int dragSourceIndex: index
                    property bool dragActive: false

                    onClicked: {
                        if (!playlistModel.showingTracks)
                            playlistModel.setCurrentPlaylistId(model.playlistId)
                        else
                            Window.window.playTrackFromPlaylist(index)
                    }

                    // Drag to reorder playlist tracks.
                    drag.target: playlistModel.showingTracks ? dragItem : null
                    drag.axis: Drag.YAxis
                    onPressed: dragActive = false
                    onPositionChanged: {
                        if (drag.active)
                            dragActive = true
                    }
                    onReleased: {
                        if (playlistModel.showingTracks && dragActive) {
                            const rowHeight = itemDelegate.height + listView.spacing
                            const localY = mapToItem(listView.contentItem, mouseX, mouseY).y
                            let target = Math.floor(localY / rowHeight)
                            target = Math.max(0, Math.min(target, playlistModel.count - 1))
                            if (target !== dragSourceIndex)
                                playlistModel.moveTrack(dragSourceIndex, target)
                        }
                        dragActive = false
                    }
                }

                Rectangle {
                    id: dragItem
                    width: itemDelegate.width
                    height: itemDelegate.height
                    color: ThemeManager.hoverColor
                    radius: ThemeManager.radiusMedium
                    opacity: itemMouse.drag.active ? 0.8 : 0
                    visible: opacity > 0

                    Drag.active: itemMouse.drag.active
                    Drag.hotSpot.x: width / 2
                    Drag.hotSpot.y: height / 2
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
                        visible: playlistModel.showingTracks

                        CrossFadeImage {
                            anchors.fill: parent
                            source: playlistModel.showingTracks && model.coverPath ? "file:///" + model.coverPath : ""
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
                            visible: !parent.children[0].visible
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Text {
                            text: playlistModel.showingTracks
                                  ? (model.title || qsTr("Unknown title"))
                                  : (model.name || qsTr("Unnamed playlist"))
                            color: ThemeManager.textColor
                            font.pixelSize: ThemeManager.fontSizeLarge
                            font.bold: true
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Text {
                            text: playlistModel.showingTracks
                                  ? ((model.artist || qsTr("Unknown artist")) + " · " + (model.album || ""))
                                  : qsTr("%1 songs").arg(model.trackCount)
                            color: ThemeManager.secondaryTextColor
                            font.pixelSize: ThemeManager.fontSizeMedium
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    Text {
                        text: playlistModel.showingTracks ? model.durationText : ""
                        color: ThemeManager.secondaryTextColor
                        font.pixelSize: ThemeManager.fontSizeMedium
                    }

                    Button {
                        text: "✎"
                        flat: true
                        visible: !playlistModel.showingTracks
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
                        onClicked: renameDialog.openFor(model.playlistId, model.name)
                    }

                    Button {
                        text: "🗑"
                        flat: true
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
                        onClicked: {
                            if (playlistModel.showingTracks)
                                playlistModel.removeTrack(playlistModel.currentPlaylistId, model.playlistId)
                            else
                                playlistModel.deletePlaylist(model.playlistId)
                        }
                    }
                }
            }
        }
    }

    Dialog {
        id: renameDialog
        anchors.centerIn: parent
        title: qsTr("Rename playlist")
        standardButtons: Dialog.Ok | Dialog.Cancel

        property int playlistId: -1

        TextField {
            id: renameField
            width: 240
            placeholderText: qsTr("Playlist name")
            color: ThemeManager.textColor
            background: Rectangle {
                color: ThemeManager.panelColor
                radius: ThemeManager.radiusSmall
            }
        }

        function openFor(id, name) {
            playlistId = id
            renameField.text = name
            open()
        }

        onAccepted: {
            if (playlistId >= 0 && renameField.text.trim().length > 0)
                playlistModel.renamePlaylist(playlistId, renameField.text)
        }
    }
}
