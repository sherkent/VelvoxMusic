import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

import VelvoxMusic

Window {
    id: rootWindow

    visible: true
    width: 1280
    height: 720
    minimumWidth: 900
    minimumHeight: 600

    flags: Qt.Window | Qt.FramelessWindowHint
    color: "transparent"
    title: qsTr("Velvox Music")

    property int currentPageIndex: 0
    property bool miniPlayerMode: false
    property bool reduceMotion: false
    property bool lyricsVisible: false
    property int currentTrackSourceRow: -1
    property string currentTrackSource: "library"
    property var currentTrack: ({
        id: -1,
        title: "",
        artist: "",
        coverPath: "",
        filePath: "",
        durationMs: 0
    })

    Connections {
        target: windowsIntegration
        function onMediaPlayPauseRequested() { rootWindow.togglePlayPause() }
        function onMediaPreviousRequested() { rootWindow.playPrevious() }
        function onMediaNextRequested() { rootWindow.playNext() }
    }

    Connections {
        target: audioEngine
        function onFinished() { rootWindow.playNext() }
    }

    onReduceMotionChanged: Motion.reduceMotion = rootWindow.reduceMotion

    function playTrack(row) {
        let idx = trackListModel.index(row, 0)
        let filePath = trackListModel.data(idx, TrackListModel.FilePathRole)

        rootWindow.currentTrack = {
            id: trackListModel.data(idx, TrackListModel.IdRole) || -1,
            title: trackListModel.data(idx, TrackListModel.TitleRole) || qsTr("Unknown title"),
            artist: trackListModel.data(idx, TrackListModel.ArtistRole) || qsTr("Unknown artist"),
            coverPath: trackListModel.data(idx, TrackListModel.CoverPathRole) || "",
            filePath: filePath,
            durationMs: trackListModel.data(idx, TrackListModel.DurationMsRole) || 0
        }
        rootWindow.currentTrackSource = "library"
        rootWindow.currentTrackSourceRow = row

        lyricsModel.loadForTrack(filePath)
        audioEngine.play(filePath)
    }

    function playTrackFromPlaylist(row) {
        let idx = playlistModel.index(row, 0)
        let filePath = playlistModel.data(idx, PlaylistModel.FilePathRole)
        let title = playlistModel.data(idx, PlaylistModel.TitleRole) || qsTr("Unknown title")
        let artist = playlistModel.data(idx, PlaylistModel.ArtistRole) || qsTr("Unknown artist")
        let coverPath = playlistModel.data(idx, PlaylistModel.CoverPathRole) || ""
        let durationMs = playlistModel.data(idx, PlaylistModel.DurationMsRole) || 0

        rootWindow.currentTrack = {
            id: playlistModel.data(idx, PlaylistModel.IdRole) || -1,
            title: title,
            artist: artist,
            coverPath: coverPath,
            filePath: filePath,
            durationMs: durationMs
        }
        rootWindow.currentTrackSource = "playlist"
        rootWindow.currentTrackSourceRow = row

        lyricsModel.loadForTrack(filePath)
        audioEngine.play(filePath)
    }

    function playTrackFromData(data) {
        let filePath = data.filePath || ""
        if (!filePath)
            return

        rootWindow.currentTrack = {
            id: data.trackId || -1,
            title: data.title || qsTr("Unknown title"),
            artist: data.artist || qsTr("Unknown artist"),
            coverPath: data.coverPath || "",
            filePath: filePath,
            durationMs: data.durationMs || 0
        }
        rootWindow.currentTrackSource = "library"
        rootWindow.currentTrackSourceRow = -1

        lyricsModel.loadForTrack(filePath)
        audioEngine.play(filePath)
    }

    function togglePlayPause() {
        if (audioEngine.playbackState === AudioEngine.Playing)
            audioEngine.pause()
        else if (audioEngine.playbackState === AudioEngine.Paused)
            audioEngine.resume()
        else if (rootWindow.currentTrack.filePath)
            audioEngine.play(rootWindow.currentTrack.filePath)
    }

    function playNext() {
        if (rootWindow.currentTrackSourceRow < 0)
            return
        let nextRow = rootWindow.currentTrackSourceRow + 1
        if (rootWindow.currentTrackSource === "playlist") {
            if (nextRow >= playlistModel.count)
                nextRow = 0
            if (playlistModel.count > 0)
                rootWindow.playTrackFromPlaylist(nextRow)
        } else {
            if (nextRow >= trackListModel.count)
                nextRow = 0
            if (trackListModel.count > 0)
                rootWindow.playTrack(nextRow)
        }
    }

    function playPrevious() {
        if (rootWindow.currentTrackSourceRow < 0)
            return
        let prevRow = rootWindow.currentTrackSourceRow - 1
        if (rootWindow.currentTrackSource === "playlist") {
            if (prevRow < 0)
                prevRow = Math.max(0, playlistModel.count - 1)
            if (playlistModel.count > 0)
                rootWindow.playTrackFromPlaylist(prevRow)
        } else {
            if (prevRow < 0)
                prevRow = Math.max(0, trackListModel.count - 1)
            if (trackListModel.count > 0)
                rootWindow.playTrack(prevRow)
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

    function toggleMaximized() {
        if (rootWindow.visibility === Window.Maximized)
            rootWindow.showNormal()
        else
            rootWindow.showMaximized()
    }

    Component { id: libraryPageComponent; LibraryPage {} }
    Component { id: albumPageComponent; AlbumPage {} }
    Component { id: artistPageComponent; ArtistPage {} }
    Component { id: playlistsPageComponent; PlaylistsPage {} }
    Component { id: searchPageComponent; SearchPage {} }
    Component { id: settingsPageComponent; SettingsPage {} }
    Component { id: nowPlayingPageComponent; NowPlayingPage {} }

    // ----------------------------------------------------------------------
    // System Acrylic/Mica backdrop
    // ----------------------------------------------------------------------
    Component.onCompleted: windowBackdrop.apply(rootWindow, WindowBackdrop.Mica)

    // ----------------------------------------------------------------------
    // Title bar
    // ----------------------------------------------------------------------
    Rectangle {
        id: titleBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: ThemeManager.titleBarHeight
        color: Qt.rgba(ThemeManager.titleBarColor.r, ThemeManager.titleBarColor.g, ThemeManager.titleBarColor.b, 0.70)
        z: 10

        MouseArea {
            anchors.fill: parent
            onPressed: rootWindow.startSystemMove()
            onDoubleClicked: rootWindow.toggleMaximized()
        }

        RowLayout {
            id: titleBarContent
            anchors.fill: parent
            anchors.leftMargin: ThemeManager.spacingMedium
            anchors.rightMargin: ThemeManager.spacingMedium
            spacing: ThemeManager.spacingSmall

            Text {
                text: qsTr("Velvox Music")
                color: ThemeManager.textColor
                font.pixelSize: ThemeManager.fontSizeLarge
                font.bold: true
                verticalAlignment: Text.AlignVCenter
                Layout.fillHeight: true
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "_"
                flat: true
                Layout.preferredWidth: 32
                Layout.preferredHeight: 28
                contentItem: Text {
                    anchors.fill: parent
                    text: parent.text
                    color: ThemeManager.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    anchors.fill: parent
                    color: parent.hovered ? Qt.rgba(1, 1, 1, 0.1) : "transparent"
                    radius: ThemeManager.radiusSmall
                    Behavior on color { ColorAnimation { duration: Motion.durationFast } }
                }
                onClicked: rootWindow.showMinimized()
            }

            Button {
                text: rootWindow.visibility === Window.Maximized ? "❐" : "□"
                flat: true
                Layout.preferredWidth: 32
                Layout.preferredHeight: 28
                contentItem: Text {
                    anchors.fill: parent
                    text: parent.text
                    color: ThemeManager.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    anchors.fill: parent
                    color: parent.hovered ? Qt.rgba(1, 1, 1, 0.1) : "transparent"
                    radius: ThemeManager.radiusSmall
                    Behavior on color { ColorAnimation { duration: Motion.durationFast } }
                }
                onClicked: rootWindow.toggleMaximized()
            }

            Button {
                text: "×"
                flat: true
                Layout.preferredWidth: 32
                Layout.preferredHeight: 28
                contentItem: Text {
                    anchors.fill: parent
                    text: parent.text
                    color: ThemeManager.textColor
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    anchors.fill: parent
                    color: parent.hovered ? Qt.rgba(1, 0, 0, 0.2) : "transparent"
                    radius: ThemeManager.radiusSmall
                    Behavior on color { ColorAnimation { duration: Motion.durationFast } }
                }
                onClicked: Qt.quit()
            }
        }
    }

    // ----------------------------------------------------------------------
    // Bottom player bar
    // ----------------------------------------------------------------------
    Rectangle {
        id: playerBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: ThemeManager.playerBarHeight
        color: Qt.rgba(ThemeManager.panelColor.r, ThemeManager.panelColor.g, ThemeManager.panelColor.b, 0.72)
        z: 10

        RowLayout {
            anchors.fill: parent
            anchors.margins: ThemeManager.spacingMedium
            spacing: ThemeManager.spacingMedium

            // Cover + info
            RowLayout {
                spacing: ThemeManager.spacingMedium
                Layout.preferredWidth: 260

                Rectangle {
                    id: coverContainer
                    Layout.preferredWidth: 56
                    Layout.preferredHeight: 56
                    color: ThemeManager.panelColor
                    radius: ThemeManager.radiusSmall
                    clip: true

                    CrossFadeImage {
                        anchors.fill: parent
                        source: rootWindow.currentTrack.coverPath
                                  ? "file:///" + rootWindow.currentTrack.coverPath
                                  : ""
                        fillMode: Image.PreserveAspectCrop
                        sourceSize: Qt.size(56, 56)
                        visible: source.length > 0
                    }

                    Text {
                        anchors.centerIn: parent
                        text: "♪"
                        color: ThemeManager.secondaryTextColor
                        font.pixelSize: 24
                        visible: rootWindow.currentTrack.coverPath.length === 0
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: stackView.push(nowPlayingPageComponent)
                    }
                }

                ColumnLayout {
                    spacing: 2
                    Layout.fillWidth: true

                    Text {
                        text: rootWindow.currentTrack.title || qsTr("No track selected")
                        color: ThemeManager.textColor
                        font.pixelSize: ThemeManager.fontSizeLarge
                        font.bold: true
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Text {
                        text: rootWindow.currentTrack.artist || ""
                        color: ThemeManager.secondaryTextColor
                        font.pixelSize: ThemeManager.fontSizeMedium
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }
            }

            // Playback controls
            RowLayout {
                spacing: ThemeManager.spacingMedium
                Layout.alignment: Qt.AlignHCenter

                Button {
                    text: "⏮"
                    flat: true
                    enabled: rootWindow.currentTrackSourceRow >= 0
                    onClicked: rootWindow.playPrevious()
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
                    onClicked: rootWindow.togglePlayPause()
                }

                Button {
                    text: "⏭"
                    flat: true
                    enabled: rootWindow.currentTrackSourceRow >= 0
                    onClicked: rootWindow.playNext()
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

                Button {
                    text: "Lyrics"
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
                    onClicked: rootWindow.lyricsVisible = !rootWindow.lyricsVisible
                }

                Button {
                    text: "Mini"
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
                    onClicked: rootWindow.miniPlayerMode = !rootWindow.miniPlayerMode
                }
            }

            // Progress
            RowLayout {
                spacing: ThemeManager.spacingSmall
                Layout.fillWidth: true

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

            // Volume
            RowLayout {
                spacing: ThemeManager.spacingSmall
                Layout.preferredWidth: 140

                Text {
                    text: "Vol"
                    color: ThemeManager.secondaryTextColor
                }

                Slider {
                    Layout.fillWidth: true
                    from: 0
                    to: 1
                    value: audioEngine.volume
                    onMoved: audioEngine.volume = value
                }
            }
        }
    }

    // ----------------------------------------------------------------------
    // Main area: navigation + content
    // ----------------------------------------------------------------------
    RowLayout {
        id: mainArea
        anchors.top: titleBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: playerBar.top
        spacing: 0
        z: 0

        // Left navigation
        Rectangle {
            id: sidePanel
            Layout.preferredWidth: ThemeManager.navWidth
            Layout.fillHeight: true
            color: Qt.rgba(ThemeManager.panelColor.r, ThemeManager.panelColor.g, ThemeManager.panelColor.b, 0.68)

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: ThemeManager.spacingMedium
                spacing: ThemeManager.spacingSmall

                Repeater {
                    model: [
                        { name: qsTr("Library"), component: libraryPageComponent, overlay: false },
                        { name: qsTr("Albums"), component: albumPageComponent, overlay: false },
                        { name: qsTr("Artists"), component: artistPageComponent, overlay: false },
                        { name: qsTr("Playlists"), component: playlistsPageComponent, overlay: false },
                        { name: qsTr("Search"), component: searchPageComponent, overlay: false },
                        { name: qsTr("Now Playing"), component: nowPlayingPageComponent, overlay: true },
                        { name: qsTr("Settings"), component: settingsPageComponent, overlay: false }
                    ]

                    delegate: Button {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        text: modelData.name
                        flat: true

                        contentItem: Text {
                            text: parent.text
                            color: ThemeManager.textColor
                            font.pixelSize: ThemeManager.fontSizeMedium
                            horizontalAlignment: Text.AlignLeft
                            verticalAlignment: Text.AlignVCenter
                        }

                        background: Rectangle {
                            color: rootWindow.currentPageIndex === index
                                    ? ThemeManager.hoverColor
                                    : (parent.down
                                        ? Qt.darker(ThemeManager.panelColor, 1.2)
                                        : (parent.hovered ? ThemeManager.panelColor : "transparent"))
                            radius: ThemeManager.radiusSmall

                            Behavior on color { ColorAnimation { duration: Motion.durationFast } }
                            Behavior on opacity { NumberAnimation { duration: Motion.durationFast; easing: Motion.easeOutCubic } }
                            opacity: parent.down ? 0.85 : 1.0
                        }

                        onClicked: {
                            if (modelData.overlay) {
                                stackView.push(modelData.component)
                            } else {
                                rootWindow.currentPageIndex = index
                                stackView.replace(modelData.component)
                            }
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }

        // Content stack
        StackView {
            id: stackView
            Layout.fillWidth: true
            Layout.fillHeight: true
            initialItem: libraryPageComponent

            pushEnter: Transition {
                ParallelAnimation {
                    NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Motion.durationNormal; easing: Motion.easeOutCubic }
                    NumberAnimation { property: "x"; from: 32; to: 0; duration: Motion.durationNormal; easing: Motion.easeOutCubic }
                }
            }
            pushExit: Transition {
                NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Motion.durationFast; easing: Motion.easeOutCubic }
            }
            popEnter: Transition {
                ParallelAnimation {
                    NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Motion.durationNormal; easing: Motion.easeOutCubic }
                    NumberAnimation { property: "x"; from: -32; to: 0; duration: Motion.durationNormal; easing: Motion.easeOutCubic }
                }
            }
            popExit: Transition {
                NumberAnimation { property: "opacity"; from: 1; to: 0; duration: Motion.durationFast; easing: Motion.easeOutCubic }
            }
            replaceEnter: pushEnter
            replaceExit: pushExit
        }
    }

    // --------------------------------------------------------------------------
    // Standalone mini player window
    // --------------------------------------------------------------------------
    MiniPlayerWindow {
        id: miniPlayerWindow
        currentTrack: rootWindow.currentTrack
        visible: rootWindow.miniPlayerMode
        onRequestRestore: {
            rootWindow.miniPlayerMode = false
            rootWindow.showNormal()
            rootWindow.raise()
            rootWindow.requestActivate()
        }
        onRequestPrevious: rootWindow.playPrevious()
        onRequestNext: rootWindow.playNext()
    }

    onMiniPlayerModeChanged: {
        if (rootWindow.miniPlayerMode) {
            rootWindow.hide()
            miniPlayerWindow.show()
            miniPlayerWindow.raise()
            miniPlayerWindow.requestActivate()
        } else {
            miniPlayerWindow.hide()
            rootWindow.show()
            rootWindow.raise()
            rootWindow.requestActivate()
        }
    }

    // --------------------------------------------------------------------------
    // Lyrics panel (slides up above the player bar)
    // --------------------------------------------------------------------------
    LyricsView {
        id: lyricsPanel
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: playerBar.top
        anchors.bottomMargin: rootWindow.lyricsVisible ? 0 : -height
        height: 280
        z: 50

        Behavior on anchors.bottomMargin {
            enabled: !Motion.reduceMotion
            NumberAnimation { duration: Motion.durationNormal; easing: Motion.easeOutCubic }
        }
    }

    // --------------------------------------------------------------------------
    // Window resize handles
    // --------------------------------------------------------------------------
    property int resizeBorder: 8

    MouseArea {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: resizeBorder
        cursorShape: Qt.SizeHorCursor
        z: 100
        property int lastX: 0
        onPressed: lastX = mouseX
        onPositionChanged: {
            if (!pressed)
                return
            let dx = mouseX - lastX
            let newW = rootWindow.width - dx
            if (newW >= rootWindow.minimumWidth) {
                rootWindow.x += dx
                rootWindow.width = newW
            }
        }
    }

    MouseArea {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: resizeBorder
        cursorShape: Qt.SizeHorCursor
        z: 100
        property int lastX: 0
        onPressed: lastX = mouseX
        onPositionChanged: {
            if (!pressed)
                return
            let newW = rootWindow.width + (mouseX - lastX)
            if (newW >= rootWindow.minimumWidth)
                rootWindow.width = newW
        }
    }

    MouseArea {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: resizeBorder
        cursorShape: Qt.SizeVerCursor
        z: 100
        property int lastY: 0
        onPressed: lastY = mouseY
        onPositionChanged: {
            if (!pressed)
                return
            let dy = mouseY - lastY
            let newH = rootWindow.height - dy
            if (newH >= rootWindow.minimumHeight) {
                rootWindow.y += dy
                rootWindow.height = newH
            }
        }
    }

    MouseArea {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: resizeBorder
        cursorShape: Qt.SizeVerCursor
        z: 100
        property int lastY: 0
        onPressed: lastY = mouseY
        onPositionChanged: {
            if (!pressed)
                return
            let newH = rootWindow.height + (mouseY - lastY)
            if (newH >= rootWindow.minimumHeight)
                rootWindow.height = newH
        }
    }
}
