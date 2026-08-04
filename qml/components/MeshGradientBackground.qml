import QtQuick
import QtQuick.Window

import VelvoxMusic

/**
 * @brief Dynamic mesh-gradient background using a precompiled RHI shader.
 *
 * Supports three render-quality tiers. At Low quality the effect is rendered at
 * 0.5x resolution and stretched, which keeps integrated GPUs happy.
 *
 * The animation is automatically paused when the containing window is minimized
 * or loses focus, and resumes when it comes back.
 *
 * Optional "rhythm mode" listens to audioEngine.spectrumChanged and gently
 * modulates blob size/brightness with a smoothed energy value.
 */
Item {
    id: root

    enum Quality { Low, Medium, High }

    property int renderQuality: MeshGradientBackground.Low
    property bool rhythmEnabled: false
    property real energy: 0.0

    readonly property bool windowActive: Window.window
            ? (Window.window.active && Window.window.visibility !== Window.Minimized)
            : true

    property bool running: windowActive

    // -------------------------------------------------------------------------
    // Quality tiers
    // -------------------------------------------------------------------------
    readonly property real _renderScale: {
        switch (renderQuality) {
        case MeshGradientBackground.Low:    return 0.5
        case MeshGradientBackground.Medium: return 0.75
        case MeshGradientBackground.High:   return 1.0
        default:                            return 0.5
        }
    }
    readonly property int _blobCount: renderQuality === MeshGradientBackground.Low ? 3 : 4
    readonly property real _baseRadius: {
        switch (renderQuality) {
        case MeshGradientBackground.Low:    return 0.52
        case MeshGradientBackground.Medium: return 0.58
        case MeshGradientBackground.High:   return 0.64
        default:                            return 0.52
        }
    }

    // -------------------------------------------------------------------------
    // Time driver (paused automatically when running == false)
    // -------------------------------------------------------------------------
    property real time: 0
    NumberAnimation on time {
        from: 0
        to: 100000
        duration: 2000000   // 50 time-units per second
        loops: Animation.Infinite
        running: root.running
    }

    // -------------------------------------------------------------------------
    // Drifting blob positions (UV space 0..1)
    // -------------------------------------------------------------------------
    readonly property real p1x: 0.28 + 0.18 * Math.sin(time * 0.40)
    readonly property real p1y: 0.35 + 0.15 * Math.cos(time * 0.30)
    readonly property real p2x: 0.72 + 0.15 * Math.sin(time * 0.35 + 2.0)
    readonly property real p2y: 0.28 + 0.18 * Math.cos(time * 0.25 + 1.0)
    readonly property real p3x: 0.35 + 0.20 * Math.sin(time * 0.30 + 4.0)
    readonly property real p3y: 0.68 + 0.15 * Math.cos(time * 0.45 + 3.0)
    readonly property real p4x: 0.65 + 0.16 * Math.sin(time * 0.50 + 5.0)
    readonly property real p4y: 0.75 + 0.12 * Math.cos(time * 0.40 + 2.5)

    // -------------------------------------------------------------------------
    // Spectrum energy smoothing
    // -------------------------------------------------------------------------
    property real _targetEnergy: 0.0
    Behavior on energy { NumberAnimation { duration: 100 } }
    on_TargetEnergyChanged: energy = _targetEnergy

    Connections {
        target: audioEngine
        function onSpectrumChanged(bands) {
            if (!root.rhythmEnabled || !bands || bands.length === 0) {
                root._targetEnergy = 0.0
                return
            }

            let sum = 0.0
            for (let i = 0; i < bands.length; ++i)
                sum += bands[i]
            const avg = sum / bands.length
            root._targetEnergy = Math.min(1.0, avg * 5.0)
        }
    }

    // -------------------------------------------------------------------------
    // Shader effect
    // -------------------------------------------------------------------------
    ShaderEffect {
        id: shader
        anchors.fill: parent

        // Render to a lower-resolution texture for Low/Medium quality.
        layer.enabled: true
        layer.textureSize: Qt.size(
            Math.max(1, Math.round(root.width  * root._renderScale)),
            Math.max(1, Math.round(root.height * root._renderScale)))
        layer.smooth: true

        property real u_time: root.time
        property size u_resolution: Qt.size(root.width, root.height)
        property int u_blobCount: root._blobCount
        property color u_baseColor: ThemeManager.gradientBaseColor
        property color u_color1: ThemeManager.gradientColor1
        property color u_color2: ThemeManager.gradientColor2
        property color u_color3: ThemeManager.gradientColor3
        property color u_color4: ThemeManager.gradientColor4
        property point u_pos1: Qt.point(root.p1x, root.p1y)
        property point u_pos2: Qt.point(root.p2x, root.p2y)
        property point u_pos3: Qt.point(root.p3x, root.p3y)
        property point u_pos4: Qt.point(root.p4x, root.p4y)
        property real u_radius1: root._baseRadius
        property real u_radius2: root._baseRadius * 0.95
        property real u_radius3: root._baseRadius * 1.05
        property real u_radius4: root._baseRadius * 0.92
        property real u_energy: root.energy
        property int u_rhythmEnabled: root.rhythmEnabled ? 1 : 0

        fragmentShader: "qrc:/meshgradient.frag.qsb"
    }
}
