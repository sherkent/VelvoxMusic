import QtQuick
import QtQuick.Effects

import VelvoxMusic

/**
 * @brief Reusable acrylic/glass panel using Qt 6 MultiEffect.
 *
 * - Blurs a region of a backing source item (typically the dynamic background).
 * - Overlays a tinted rectangle for the frosted-glass look.
 * - Supports Low/Medium/High render quality to trade blur fidelity for GPU cost.
 *
 * The source item should have layer.enabled=true so the blur does not re-render
 * the entire background every frame.
 */
Item {
    id: root

    enum Quality { Low, Medium, High }

    default property alias content: contentContainer.children

    property Item source: null
    property real blurRadius: 64
    property color tintColor: ThemeManager.panelColor
    property real tintOpacity: 0.72
    property int renderQuality: AcrylicPanel.Quality.Medium

    readonly property real _qualityScale: {
        switch (renderQuality) {
        case AcrylicPanel.Quality.Low:    return 0.5
        case AcrylicPanel.Quality.Medium: return 0.75
        case AcrylicPanel.Quality.High:   return 1.0
        default:                          return 0.75
        }
    }
    readonly property real _blurMultiplier: {
        switch (renderQuality) {
        case AcrylicPanel.Quality.Low:    return 0.6
        case AcrylicPanel.Quality.Medium: return 0.85
        case AcrylicPanel.Quality.High:   return 1.0
        default:                          return 0.85
        }
    }

    MultiEffect {
        id: blurEffect
        anchors.fill: parent
        visible: root.source !== null

        source: root.source
        blurEnabled: true
        blur: 1.0
        blurMax: root.blurRadius * root._qualityScale
        blurMultiplier: root._blurMultiplier
        autoPaddingEnabled: true

        sourceRect: root.source
            ? Qt.rect(
                root.mapToItem(root.source, 0, 0).x,
                root.mapToItem(root.source, 0, 0).y,
                root.width,
                root.height)
            : Qt.rect(0, 0, 0, 0)
    }

    Rectangle {
        anchors.fill: parent
        color: root.tintColor
        opacity: root.tintOpacity
    }

    Item {
        id: contentContainer
        anchors.fill: parent
    }
}
