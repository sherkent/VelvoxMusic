import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import VelvoxMusic

Page {
    id: root

    background: Rectangle { color: ThemeManager.backgroundColor }

    property bool scanning: false
    property int scanProcessed: 0
    property int scanTotal: 0
    property string scanFile: ""
    property string lastResult: ""

    Connections {
        target: libraryManager
        function onScanStarted() {
            root.scanning = true
            root.scanProcessed = 0
            root.scanTotal = 0
            root.scanFile = ""
            root.lastResult = ""
        }
        function onScanProgress(processed, total, currentFile) {
            root.scanProcessed = processed
            root.scanTotal = total
            root.scanFile = currentFile
        }
        function onScanFinished(added, updated, removed) {
            root.scanning = false
            root.lastResult = qsTr("Done: +%1 / ~%2 / -%3").arg(added).arg(updated).arg(removed)
        }
        function onScanCancelled() {
            root.scanning = false
            root.lastResult = qsTr("Cancelled")
        }
        function onScanError(filePath, error) {
            console.warn("Scan error for", filePath, ":", error)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 40
        spacing: 20

        Text {
            text: qsTr("Library Manager Test")
            color: ThemeManager.textColor
            font.pixelSize: 28
            font.bold: true
        }

        // Directory input
        RowLayout {
            spacing: 12
            Layout.fillWidth: true

            TextField {
                id: dirField
                Layout.fillWidth: true
                placeholderText: qsTr("C:/Music")
                text: "C:/Music"
                color: ThemeManager.textColor
            }

            AppButton {
                text: qsTr("Add directory")
                onClicked: libraryManager.addMusicDirectory(dirField.text)
            }

            AppButton {
                text: qsTr("Start scan")
                enabled: !root.scanning
                onClicked: libraryManager.startScan()
            }

            AppButton {
                text: qsTr("Cancel")
                enabled: root.scanning
                onClicked: libraryManager.cancelScan()
            }
        }

        // Progress
        ColumnLayout {
            spacing: 6
            Layout.fillWidth: true
            visible: root.scanning || root.lastResult.length > 0

            ProgressBar {
                Layout.fillWidth: true
                from: 0
                to: Math.max(root.scanTotal, 1)
                value: root.scanProcessed
                visible: root.scanning
            }

            Text {
                text: root.scanning
                      ? qsTr("Scanned %1 / %2 - %3").arg(root.scanProcessed).arg(root.scanTotal).arg(root.scanFile)
                      : root.lastResult
                color: ThemeManager.secondaryTextColor
                font.pixelSize: 14
            }
        }

        // Filter and sort
        RowLayout {
            spacing: 12
            Layout.fillWidth: true

            TextField {
                id: filterField
                Layout.fillWidth: true
                placeholderText: qsTr("Search title / artist / album...")
                color: ThemeManager.textColor
                onTextChanged: trackListModel.setFilter(text)
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
            font.pixelSize: 14
        }

        // Track list
        ListView {
            id: trackList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: trackListModel
            spacing: 8
            cacheBuffer: Math.max(200, height * 1.5)

            delegate: Rectangle {
                width: trackList.width
                height: 72
                color: "#2a2a40"
                radius: 8

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 12

                    Image {
                        source: coverPath ? "file:///" + coverPath : ""
                        Layout.preferredWidth: 52
                        Layout.preferredHeight: 52
                        sourceSize: Qt.size(52, 52)
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        visible: coverPath.length > 0
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Text {
                            text: title || qsTr("Unknown title")
                            color: ThemeManager.textColor
                            font.pixelSize: 16
                            font.bold: true
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Text {
                            text: (artist || qsTr("Unknown artist")) + " - " + (album || qsTr("Unknown album"))
                            color: ThemeManager.secondaryTextColor
                            font.pixelSize: 13
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    Text {
                        text: durationText
                        color: ThemeManager.secondaryTextColor
                        Layout.preferredWidth: 50
                    }

                    AppButton {
                        text: isFavorite ? qsTr("Unfavorite") : qsTr("Favorite")
                        onClicked: trackListModel.toggleFavorite(index)
                    }

                    AppButton {
                        text: qsTr("Play")
                        onClicked: audioEngine.play(filePath)
                    }

                    AppButton {
                        text: qsTr("Remove")
                        onClicked: trackListModel.removeTrack(index)
                    }
                }
            }

            ScrollBar.vertical: ScrollBar {}
        }
    }
}
