#include "core/WindowsSystemIntegration.h"
#include "core/AudioEngine.h"

#include <QAbstractNativeEventFilter>
#include <QApplication>
#include <QWindow>
#include <QDebug>
#include <QTimer>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#include <shobjidl.h>
#include <qt_windows.h>
#include <QNativeInterface>
#endif

namespace Velvox {

#ifdef Q_OS_WIN
class WindowsSystemIntegration::Impl : public QAbstractNativeEventFilter
{
public:
    WindowsSystemIntegration *q = nullptr;
    AudioEngine *audioEngine = nullptr;
    QObject *eventTarget = nullptr;
    ITaskbarList3 *taskbarList = nullptr;
    HWND hwnd = nullptr;
    bool buttonsCreated = false;
    QVector<HBITMAP> bitmaps;

    explicit Impl(WindowsSystemIntegration *parent)
        : q(parent)
    {
    }

    ~Impl()
    {
        if (taskbarList) {
            taskbarList->Release();
            taskbarList = nullptr;
        }
        for (HBITMAP bmp : std::as_const(bitmaps)) {
            if (bmp)
                DeleteObject(bmp);
        }
        bitmaps.clear();
    }

    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override
    {
        Q_UNUSED(eventType)
        MSG *msg = static_cast<MSG *>(message);

        if (msg->message == WM_APPCOMMAND) {
            const int cmd = GET_APPCOMMAND_LPARAM(msg->lParam);
            if (cmd == APPCOMMAND_MEDIA_PLAY_PAUSE) {
                emit q->mediaPlayPauseRequested();
                if (result) *result = TRUE;
                return true;
            } else if (cmd == APPCOMMAND_MEDIA_STOP) {
                if (audioEngine) audioEngine->stop();
                if (result) *result = TRUE;
                return true;
            } else if (cmd == APPCOMMAND_MEDIA_NEXTTRACK) {
                emit q->mediaNextRequested();
                if (result) *result = TRUE;
                return true;
            } else if (cmd == APPCOMMAND_MEDIA_PREVIOUSTRACK) {
                emit q->mediaPreviousRequested();
                if (result) *result = TRUE;
                return true;
            }
        }

        if (msg->message == WM_COMMAND && HIWORD(msg->wParam) == THBN_CLICKED) {
            const int buttonId = LOWORD(msg->wParam);
            if (buttonId == 100) {
                emit q->mediaPreviousRequested();
            } else if (buttonId == 101) {
                emit q->mediaPlayPauseRequested();
            } else if (buttonId == 102) {
                emit q->mediaNextRequested();
            }
            if (result) *result = 0;
            return true;
        }

        return false;
    }

    void createTaskbarButtons(HWND window)
    {
        if (!window || buttonsCreated)
            return;

        HRESULT hr = CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER,
                                      IID_ITaskbarList3, reinterpret_cast<void **>(&taskbarList));
        if (FAILED(hr)) {
            qWarning() << "Failed to create ITaskbarList3:" << hr;
            return;
        }

        taskbarList->HrInit();

        // Build simple bitmaps for each button. In production use themed icons.
        HBITMAP hPrev = createSolidBitmap(24, 24, RGB(200, 200, 200));
        HBITMAP hPlay = createSolidBitmap(24, 24, RGB(0, 200, 0));
        HBITMAP hNext = createSolidBitmap(24, 24, RGB(200, 200, 200));
        bitmaps << hPrev << hPlay << hNext;

        THUMBBUTTON buttons[3] = {};
        buttons[0].dwMask = THB_BITMAP | THB_TOOLTIP;
        buttons[0].iId = 100;
        buttons[0].hBitmap = hPrev;
        wcscpy_s(buttons[0].szTip, L"Previous");

        buttons[1].dwMask = THB_BITMAP | THB_TOOLTIP;
        buttons[1].iId = 101;
        buttons[1].hBitmap = hPlay;
        wcscpy_s(buttons[1].szTip, L"Play/Pause");

        buttons[2].dwMask = THB_BITMAP | THB_TOOLTIP;
        buttons[2].iId = 102;
        buttons[2].hBitmap = hNext;
        wcscpy_s(buttons[2].szTip, L"Next");

        hr = taskbarList->ThumbBarAddButtons(window, 3, buttons);
        if (FAILED(hr)) {
            qWarning() << "ThumbBarAddButtons failed:" << hr;
        } else {
            buttonsCreated = true;
        }
    }

    HBITMAP createSolidBitmap(int w, int h, COLORREF color)
    {
        HDC hdc = GetDC(nullptr);
        HDC memDC = CreateCompatibleDC(hdc);
        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = w;
        bmi.bmiHeader.biHeight = -h; // top-down
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        void *bits = nullptr;
        HBITMAP hBmp = CreateDIBSection(memDC, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
        if (bits) {
            DWORD *pixels = static_cast<DWORD *>(bits);
            for (int i = 0; i < w * h; ++i)
                pixels[i] = (0xFF000000) | color;
        }
        DeleteDC(memDC);
        ReleaseDC(nullptr, hdc);
        return hBmp;
    }
};
#else
class WindowsSystemIntegration::Impl : public QAbstractNativeEventFilter
{
public:
    WindowsSystemIntegration *q = nullptr;
    AudioEngine *audioEngine = nullptr;

    explicit Impl(WindowsSystemIntegration *parent)
        : q(parent)
    {
    }

    bool nativeEventFilter(const QByteArray &, void *, qintptr *) override { return false; }
    void createTaskbarButtons(void *) {}
};
#endif

WindowsSystemIntegration::WindowsSystemIntegration(QObject *parent)
    : QObject(parent)
    , d(std::make_unique<Impl>(this))
{
}

WindowsSystemIntegration::~WindowsSystemIntegration() = default;

bool WindowsSystemIntegration::install(QObject *nativeEventTarget, AudioEngine *audioEngine)
{
    d->audioEngine = audioEngine;
    d->eventTarget = nativeEventTarget;

    if (nativeEventTarget) {
        qApp->installNativeEventFilter(d.get());
    }

#ifdef Q_OS_WIN
    // Defer taskbar button creation until the main window is shown.
    QTimer::singleShot(0, this, [this]() {
        for (QWindow *window : qApp->topLevelWindows()) {
            if (QNativeInterface::QWindowsWindow *ww = window->nativeInterface<QNativeInterface::QWindowsWindow>()) {
                d->hwnd = reinterpret_cast<HWND>(ww->handle());
                if (d->hwnd)
                    d->createTaskbarButtons(d->hwnd);
                break;
            }
        }
    });
#endif

    return true;
}

} // namespace Velvox
