import QtQuick
import QtQuick.Controls

import VelvoxMusic

/**
 * @brief Play / pause button with a cross-fade + scale transition.
 *
 * Instead of snapping the icon, the outgoing icon fades and shrinks while the
 * incoming icon fades and grows. The background follows the same hover / press
 * feedback as AppButton.
 */
Button {
    id: control

    property bool playing: false

    flat: true

    contentItem: Item {
        implicitWidth: 32
        implicitHeight: 32

        Text {
            id: playIcon
            anchors.centerIn: parent
            text: "▶"
            color: ThemeManager.textColor
            font.pixelSize: ThemeManager.fontSizeLarge
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            opacity: control.playing ? 0.0 : 1.0
            scale: control.playing ? 0.7 : 1.0
            visible: opacity > 0.01

            Behavior on opacity { NumberAnimation { duration: Motion.durationFast; easing: Motion.easeOutCubic } }
            Behavior on scale  { NumberAnimation { duration: Motion.durationFast; easing: Motion.easeOutBack } }
        }

        Text {
            id: pauseIcon
            anchors.centerIn: parent
            text: "⏸"
            color: ThemeManager.textColor
            font.pixelSize: ThemeManager.fontSizeLarge
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            opacity: control.playing ? 1.0 : 0.0
            scale: control.playing ? 1.0 : 0.7
            visible: opacity > 0.01

            Behavior on opacity { NumberAnimation { duration: Motion.durationFast; easing: Motion.easeOutCubic } }
            Behavior on scale  { NumberAnimation { duration: Motion.durationFast; easing: Motion.easeOutBack } }
        }
    }

    background: Rectangle {
        color: control.down
                ? Qt.darker(ThemeManager.accentColor, 1.2)
                : (control.hovered ? Qt.lighter(ThemeManager.accentColor, 1.1) : ThemeManager.accentColor)
        radius: ThemeManager.radiusSmall

        Behavior on color { ColorAnimation { duration: Motion.durationFast } }
    }
}
