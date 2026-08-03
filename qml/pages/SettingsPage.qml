import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

import VelvoxMusic

Page {
    id: root

    background: Rectangle { color: ThemeManager.surfaceColor }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: ThemeManager.spacingLarge
        spacing: ThemeManager.spacingMedium

        Text {
            text: qsTr("Settings")
            color: ThemeManager.textColor
            font.pixelSize: ThemeManager.fontSizeTitle
            font.bold: true
        }

        RowLayout {
            spacing: ThemeManager.spacingMedium

            Text {
                text: qsTr("Render quality")
                color: ThemeManager.textColor
                font.pixelSize: ThemeManager.fontSizeLarge
            }

            ComboBox {
                // This single setting controls both the mesh-gradient background
                // resolution and the acrylic blur sampling quality.
                model: ListModel {
                    ListElement { text: "Low (0.5x)"; value: AcrylicPanel.Quality.Low }
                    ListElement { text: "Medium (0.75x)"; value: AcrylicPanel.Quality.Medium }
                    ListElement { text: "High (1.0x)"; value: AcrylicPanel.Quality.High }
                }
                textRole: "text"
                valueRole: "value"
                currentIndex: Window.window ? Window.window.backgroundQuality : 0
                onActivated: {
                    if (Window.window)
                        Window.window.backgroundQuality = currentValue
                }
            }
        }

        RowLayout {
            spacing: ThemeManager.spacingMedium

            Text {
                text: qsTr("Reduce motion")
                color: ThemeManager.textColor
                font.pixelSize: ThemeManager.fontSizeLarge
            }

            Switch {
                checked: Window.window ? Window.window.reduceMotion : false
                onToggled: {
                    if (Window.window)
                        Window.window.reduceMotion = checked
                }
            }
        }

        Text {
            text: qsTr("More settings will be implemented in a later stage.")
            color: ThemeManager.secondaryTextColor
            font.pixelSize: ThemeManager.fontSizeMedium
        }
    }
}
