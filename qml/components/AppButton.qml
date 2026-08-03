import QtQuick
import QtQuick.Controls

import VelvoxMusic

Button {
    id: control

    contentItem: Text {
        text: control.text
        font: control.font
        color: ThemeManager.textColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter

        Behavior on color { ColorAnimation { duration: Motion.durationFast } }
    }

    background: Rectangle {
        id: bg
        color: control.down
                ? Qt.darker(ThemeManager.accentColor, 1.2)
                : (control.hovered ? Qt.lighter(ThemeManager.accentColor, 1.1) : ThemeManager.accentColor)
        radius: ThemeManager.radiusSmall

        Behavior on color { ColorAnimation { duration: Motion.durationFast } }
        Behavior on scale { NumberAnimation { duration: Motion.durationFast; easing: Motion.easeOutCubic } }
        scale: control.down ? 0.97 : 1.0
    }
}
