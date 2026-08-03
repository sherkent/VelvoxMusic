pragma Singleton
import QtQuick

QtObject {
    // --------------------------------------------------------------------------
    // 色板占位（后续统一替换为渐变/玻璃拟态风格）
    // --------------------------------------------------------------------------
    readonly property color backgroundColor: "#1a1a2e"
    readonly property color surfaceColor: "#22223b"
    readonly property color panelColor: "#2a2a40"
    readonly property color primaryColor: "#e94560"
    readonly property color accentColor: "#ff6b6b"
    readonly property color textColor: "#ffffff"
    readonly property color secondaryTextColor: "#a0a0a0"
    readonly property color disabledTextColor: "#666666"
    readonly property color dividerColor: "#3a3a55"
    readonly property color titleBarColor: "#16162a"
    readonly property color hoverColor: "#353550"

    // 动态网格渐变占位色板
    readonly property color gradientBaseColor: "#0f0f1a"
    readonly property color gradientColor1: "#2d1b4e"
    readonly property color gradientColor2: "#5e2a4f"
    readonly property color gradientColor3: "#1e3a5f"
    readonly property color gradientColor4: "#4a1c40"

    // --------------------------------------------------------------------------
    // 设计 token（视觉特效阶段会统一调整）
    // --------------------------------------------------------------------------
    readonly property int radiusSmall: 4
    readonly property int radiusMedium: 8
    readonly property int radiusLarge: 12

    readonly property int spacingSmall: 6
    readonly property int spacingMedium: 12
    readonly property int spacingLarge: 20

    readonly property int titleBarHeight: 40
    readonly property int navWidth: 180
    readonly property int playerBarHeight: 80

    readonly property int fontSizeSmall: 12
    readonly property int fontSizeMedium: 14
    readonly property int fontSizeLarge: 16
    readonly property int fontSizeTitle: 24
}
