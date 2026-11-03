#pragma once

#include <QObject>

#include <windows.h>

class QTimer;

/// Watches window lifecycle events and emits a debounced refresh request.
class WinEventWatcher : public QObject
{
    Q_OBJECT

public:
    explicit WinEventWatcher(QObject *parent = nullptr);
    ~WinEventWatcher() override;

    void start();
    void stop();

signals:
    void windowActivity();

private:
    static void CALLBACK onWinEvent(HWINEVENTHOOK hook, DWORD event, HWND hwnd,
                                    LONG object, LONG child, DWORD thread, DWORD time);

    void scheduleRefresh();

    static WinEventWatcher *s_instance;

    QTimer *m_debounce = nullptr;
    QVector<HWINEVENTHOOK> m_hooks;
};
