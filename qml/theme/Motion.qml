pragma Singleton
import QtQuick

/**
 * @brief Global motion design tokens.
 *
 * All animations should pull durations and easing curves from here so the
 * entire app feels consistent. Set reduceMotion to true to disable decorative
 * animations for accessibility or low-power devices.
 */
QtObject {
    property bool reduceMotion: false

    // -------------------------------------------------------------------------
    // Durations (ms). All collapse to 0 when reduceMotion is enabled.
    // -------------------------------------------------------------------------
    readonly property int durationInstant: 0
    readonly property int durationFast:    reduceMotion ? 0 : 120
    readonly property int durationNormal:  reduceMotion ? 0 : 250
    readonly property int durationSlow:    reduceMotion ? 0 : 400

    // -------------------------------------------------------------------------
    // Common easing curves.
    // -------------------------------------------------------------------------
    readonly property var easeLinear:       Easing.Linear
    readonly property var easeOutCubic:     Easing.OutCubic
    readonly property var easeOutExpo:      Easing.OutExpo
    readonly property var easeInOutCubic:   Easing.InOutCubic
    readonly property var easeOutBack:      Easing.OutBack
    readonly property var easeSpring:       Easing.OutBack
}
