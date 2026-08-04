#include "core/WindowBackdrop.h"

#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>
#include <qt_windows.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif

// Provide fallback constants for older SDKs that don't define the backdrop enum.
namespace {
constexpr int kDwmsbtAuto = 0;
constexpr int kDwmsbtNone = 1;
constexpr int kDwmsbtMainWindow = 2;
constexpr int kDwmsbtTransientWindow = 3;
constexpr int kDwmsbtTabbedWindow = 4;
}
#endif

namespace Velvox {

WindowBackdrop::WindowBackdrop(QObject *parent)
    : QObject(parent)
{
}

void WindowBackdrop::apply(QWindow *window, BackdropType type)
{
    if (!window) {
        qWarning() << "WindowBackdrop::apply called with null window";
        return;
    }

#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(window->winId());
    if (!hwnd) {
        qWarning() << "WindowBackdrop::apply: window has no native handle";
        return;
    }

    // Dark title bar so the system material matches the app theme.
    BOOL dark = TRUE;
    HRESULT hr = DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
    if (FAILED(hr)) {
        qWarning() << "WindowBackdrop: failed to set dark mode:" << hr;
    }

    int backdrop = kDwmsbtMainWindow;
    switch (type) {
    case Acrylic:
        backdrop = kDwmsbtTabbedWindow;
        break;
    case Mica:
        backdrop = kDwmsbtMainWindow;
        break;
    case MicaAlt:
        backdrop = kDwmsbtTransientWindow;
        break;
    }

    hr = DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdrop, sizeof(backdrop));
    if (FAILED(hr)) {
        qWarning() << "WindowBackdrop: failed to set system backdrop:" << hr;
    }
#else
    Q_UNUSED(type)
#endif
}

} // namespace Velvox
