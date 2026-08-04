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

// ---------------------------------------------------------------------------
// SetWindowCompositionAttribute — real-time acrylic blur
// (shows what's actually behind the window, not just the wallpaper)
// ---------------------------------------------------------------------------
enum ACCENT_STATE {
    ACCENT_DISABLED = 0,
    ACCENT_ENABLE_GRADIENT = 1,
    ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
    ACCENT_ENABLE_BLURBEHIND = 3,
    ACCENT_ENABLE_ACRYLICBLURBEHIND = 4,
    ACCENT_ENABLE_HOSTBACKDROP = 5,
    ACCENT_INVALID_STATE = 6
};

struct ACCENTPOLICY {
    ACCENT_STATE AccentState;
    int AccentFlags;
    int GradientColor;   // ABGR format
    int AnimationId;
};

enum WINDOWCOMPOSITIONATTRIB {
    WCA_ACCENT_POLICY = 19
};

struct WINCOMPATTRDATA {
    WINDOWCOMPOSITIONATTRIB Attribute;
    PVOID Data;
    ULONG DataSize;
};

using pSetWindowCompositionAttribute = BOOL (WINAPI *)(HWND, WINCOMPATTRDATA *);

static bool applyBlurBehind(HWND hwnd)
{
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (!hUser32)
        return false;

    auto setCompAttr = reinterpret_cast<pSetWindowCompositionAttribute>(
        GetProcAddress(hUser32, "SetWindowCompositionAttribute"));
    if (!setCompAttr)
        return false;

    ACCENTPOLICY policy = {};
    policy.AccentState = ACCENT_ENABLE_ACRYLICBLURBEHIND;
    policy.AccentFlags = 2; // draw all borders
    policy.GradientColor = 0x99000000; // dark tint, ~60% opacity

    WINCOMPATTRDATA data = {};
    data.Attribute = WCA_ACCENT_POLICY;
    data.Data = &policy;
    data.DataSize = sizeof(policy);

    return setCompAttr(hwnd, &data);
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

    // BlurBehind uses SetWindowCompositionAttribute for real-time acrylic blur.
    // Fall back to DWM TabbedWindow acrylic on failure.
    if (type == BlurBehind) {
        if (applyBlurBehind(hwnd)) {
            return;
        }
        qWarning() << "WindowBackdrop: BlurBehind failed, falling back to DWM acrylic";
        type = Acrylic;
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
    default:
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
