#pragma once

#include <QObject>
#include <memory>

namespace Velvox {

class AudioEngine;

/**
 * @brief Windows-only integration: media keys and taskbar thumbnail toolbar.
 *
 * Qt6 does not provide public APIs for either feature, so we use a native
 * event filter on Windows. All platform-specific code is isolated here.
 *
 * Known Qt6 pitfall: global media keys are not delivered through QShortcut or
 * QHotkey on Windows; you must handle WM_APPCOMMAND or register a low-level
 * keyboard hook. WM_APPCOMMAND is cleaner and does not require extra DLLs.
 *
 * Taskbar thumbnail toolbar uses the ITaskbarList3 COM interface. The window
 * handle is obtained from the QWindow via QWindow::winId().
 */
class WindowsSystemIntegration : public QObject
{
    Q_OBJECT

public:
    explicit WindowsSystemIntegration(QObject *parent = nullptr);
    ~WindowsSystemIntegration() override;

    bool install(QObject *nativeEventTarget, AudioEngine *audioEngine);

signals:
    void mediaPlayPauseRequested();
    void mediaPreviousRequested();
    void mediaNextRequested();

private:
    class Impl;
    std::unique_ptr<Impl> d;
};

} // namespace Velvox
