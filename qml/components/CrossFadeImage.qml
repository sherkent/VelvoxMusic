import QtQuick

import VelvoxMusic

/**
 * @brief Image that cross-fades between old and new sources.
 *
 * The outgoing image fades out and shrinks slightly while the incoming image
 * fades in and zooms from 1.05x to 1x. When reduceMotion is enabled the switch
 * is instantaneous.
 */
Item {
    id: root

    property string source: ""
    property int fillMode: Image.PreserveAspectCrop
    property var sourceSize: undefined   // Qt.size(width, height) or undefined
    property bool animate: true          // disable in list delegates to avoid scroll jank

    property string _oldSource: ""
    property string _newSource: source

    onSourceChanged: {
        if (source === _newSource)
            return
        _oldSource = _newSource
        _newSource = source
        if (root.animate && !Motion.reduceMotion && _oldSource.length > 0)
            crossfadeAnim.restart()
        else
            root._oldSource = ""
    }

    Image {
        id: backImg
        anchors.fill: parent
        source: root._oldSource
        fillMode: root.fillMode
        sourceSize: root.sourceSize
        asynchronous: true
        opacity: 0
        scale: 1
    }

    Image {
        id: frontImg
        anchors.fill: parent
        source: root._newSource
        fillMode: root.fillMode
        sourceSize: root.sourceSize
        asynchronous: true
        opacity: 1
        scale: 1
    }

    ParallelAnimation {
        id: crossfadeAnim

        NumberAnimation { target: backImg; property: "opacity"; from: 1; to: 0; duration: Motion.durationNormal; easing: Motion.easeOutCubic }
        NumberAnimation { target: backImg; property: "scale"; from: 1; to: 0.98; duration: Motion.durationNormal; easing: Motion.easeOutCubic }
        NumberAnimation { target: frontImg; property: "opacity"; from: 0; to: 1; duration: Motion.durationNormal; easing: Motion.easeOutCubic }
        NumberAnimation { target: frontImg; property: "scale"; from: 1.05; to: 1; duration: Motion.durationNormal; easing: Motion.easeOutCubic }

        onStopped: root._oldSource = ""
    }
}
