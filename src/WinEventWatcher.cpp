#include "WinEventWatcher.h"

#include <QTimer>
#include <QVector>

#include <array>

WinEventWatcher *WinEventWatcher::s_instance = nullptr;

WinEventWatcher::WinEventWatcher(QObject *parent)
    : QObject(parent)
    , m_debounce(new QTimer(this))
{
    m_debounce->setSingleShot(true);
    m_debounce->setInterval(250);
    connect(m_debounce, &QTimer::timeout, this, &WinEventWatcher::windowActivity);
    s_instance = this;
}

WinEventWatcher::~WinEventWatcher()
{
    stop();
    if (s_instance == this)
    {
        s_instance = nullptr;
    }
}

void WinEventWatcher::start()
{
    if (!m_hooks.isEmpty())
    {
        return;
    }

    const std::array<DWORD, 6> events = {
        EVENT_OBJECT_CREATE,
        EVENT_OBJECT_DESTROY,
        EVENT_OBJECT_SHOW,
        EVENT_OBJECT_HIDE,
        EVENT_OBJECT_NAMECHANGE,
        EVENT_SYSTEM_FOREGROUND,
    };

    for (const DWORD event : events)
    {
        const HWINEVENTHOOK hook = SetWinEventHook(event, event, nullptr, &WinEventWatcher::onWinEvent,
                                                   0, 0, WINEVENT_OUTOFCONTEXT);
        if (hook != nullptr)
        {
            m_hooks.append(hook);
        }
    }
}

void WinEventWatcher::stop()
{
    for (const HWINEVENTHOOK hook : m_hooks)
    {
        UnhookWinEvent(hook);
    }
    m_hooks.clear();
}

void CALLBACK WinEventWatcher::onWinEvent(HWINEVENTHOOK hook, DWORD event, HWND hwnd, LONG object,
                                          LONG child, DWORD thread, DWORD time)
{
    Q_UNUSED(hook);
    Q_UNUSED(event);
    Q_UNUSED(hwnd);
    Q_UNUSED(object);
    Q_UNUSED(child);
    Q_UNUSED(thread);
    Q_UNUSED(time);

    if (s_instance != nullptr)
    {
        s_instance->scheduleRefresh();
    }
}

void WinEventWatcher::scheduleRefresh()
{
    if (m_debounce != nullptr)
    {
        m_debounce->start();
    }
}
