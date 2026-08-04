#pragma once

#include <QObject>
#include <QWindow>

namespace Velvox {

/**
 * @brief Applies the system Acrylic / Mica backdrop to a top-level window.
 *
 * On Windows 11 this uses DWMWA_SYSTEMBACKDROP_TYPE. On older Windows or other
 * platforms the call is a no-op, leaving the window transparent so the normal
 * background color shows through.
 */
class WindowBackdrop : public QObject
{
    Q_OBJECT

public:
    explicit WindowBackdrop(QObject *parent = nullptr);

    enum BackdropType {
        Acrylic,   ///< Tabbed-window acrylic (strong blur + tint)
        Mica,      ///< Main-window mica (subtle desktop-tinted material)
        MicaAlt    ///< Transient-window mica variant
    };
    Q_ENUM(BackdropType)

    Q_INVOKABLE void apply(QWindow *window, BackdropType type = Mica);
};

} // namespace Velvox
