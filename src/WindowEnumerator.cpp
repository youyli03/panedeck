#include "WindowEnumerator.h"

#include "PathResolver.h"

#include <QFileIconProvider>
#include <QFileInfo>
#include <QHash>
#include <QSet>

#include <windows.h>
#include <dwmapi.h>
#include <winuser.h>

#include <cstdio>
#include <iterator>
#include <vector>
#include <winver.h>

namespace
{
constexpr DWORD kProcessQueryLimitedInformation = 0x1000;
constexpr DWORD kDwmwaCloaked = 14;

QString windowText(HWND hwnd)
{
    const int length = GetWindowTextLengthW(hwnd);
    if (length <= 0)
    {
        return {};
    }

    std::wstring buffer(static_cast<size_t>(length + 1), L'\0');
    const int copied = GetWindowTextW(hwnd, buffer.data(), length + 1);
    if (copied <= 0)
    {
        return {};
    }

    return QString::fromWCharArray(buffer.data(), copied).trimmed();
}

QString className(HWND hwnd)
{
    wchar_t buffer[256] = {};
    const int copied = GetClassNameW(hwnd, buffer, static_cast<int>(std::size(buffer)));
    return copied > 0 ? QString::fromWCharArray(buffer, copied) : QString();
}

QString fileDescription(const QString &path)
{
    const std::wstring widePath = path.toStdWString();
    DWORD ignored = 0;
    const DWORD size = GetFileVersionInfoSizeW(widePath.c_str(), &ignored);
    if (size == 0)
    {
        return {};
    }

    std::vector<BYTE> data(size);
    if (!GetFileVersionInfoW(widePath.c_str(), 0, size, data.data()))
    {
        return {};
    }

    struct Translation
    {
        WORD language;
        WORD codePage;
    };

    Translation *translation = nullptr;
    UINT translationSize = 0;
    if (!VerQueryValueW(data.data(), L"\\VarFileInfo\\Translation",
                        reinterpret_cast<LPVOID *>(&translation), &translationSize)
        || translationSize < sizeof(Translation) || translation == nullptr)
    {
        return {};
    }

    wchar_t query[128] = {};
    swprintf(query, 128, L"\\StringFileInfo\\%04x%04x\\FileDescription",
             translation[0].language, translation[0].codePage);

    wchar_t *description = nullptr;
    UINT descriptionLength = 0;
    if (VerQueryValueW(data.data(), query, reinterpret_cast<LPVOID *>(&description),
                       &descriptionLength)
        && descriptionLength > 0 && description != nullptr)
    {
        return QString::fromWCharArray(description).trimmed();
    }

    return {};
}

QString processPath(DWORD processId)
{
    HANDLE process = OpenProcess(kProcessQueryLimitedInformation, FALSE, processId);
    if (process == nullptr)
    {
        return {};
    }

    wchar_t buffer[MAX_PATH * 4] = {};
    DWORD size = static_cast<DWORD>(std::size(buffer));
    QString result;
    if (QueryFullProcessImageNameW(process, 0, buffer, &size))
    {
        result = QString::fromWCharArray(buffer, static_cast<int>(size));
    }

    CloseHandle(process);
    return result;
}

bool isWindowCloaked(HWND hwnd)
{
    BOOL cloaked = FALSE;
    if (SUCCEEDED(DwmGetWindowAttribute(hwnd, kDwmwaCloaked, &cloaked, sizeof(cloaked))))
    {
        return cloaked != FALSE;
    }
    return false;
}

bool isExcludedClass(const QString &name)
{
    static const QSet<QString> excluded = {
        QStringLiteral("Progman"),
        QStringLiteral("WorkerW"),
        QStringLiteral("Shell_TrayWnd"),
        QStringLiteral("Shell_SecondaryTrayWnd"),
        QStringLiteral("Windows.UI.Core.CoreWindow"),
        QStringLiteral("ApplicationFrameWindow"),
    };
    return excluded.contains(name);
}

struct CaptureContext
{
    QVector<WindowInfo> *windows = nullptr;
    PathResolver *resolver = nullptr;
    QFileIconProvider *icons = nullptr;
    QHash<QString, QIcon> *iconCache = nullptr;
};

BOOL CALLBACK collectWindow(HWND hwnd, LPARAM parameter)
{
    auto *context = reinterpret_cast<CaptureContext *>(parameter);

    if (!IsWindowVisible(hwnd))
    {
        return TRUE;
    }

    const bool minimized = IsIconic(hwnd) != FALSE;

    const LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    const LONG_PTR extendedStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if ((extendedStyle & WS_EX_TOOLWINDOW) != 0
        || (extendedStyle & WS_EX_NOACTIVATE) != 0
        || (style & WS_CHILD) != 0)
    {
        return TRUE;
    }

    const HWND owner = GetWindow(hwnd, GW_OWNER);
    if (owner != nullptr && (extendedStyle & WS_EX_APPWINDOW) == 0)
    {
        return TRUE;
    }

    const QString title = windowText(hwnd);
    const QString cls = className(hwnd);
    if (title.isEmpty() || isExcludedClass(cls) || isWindowCloaked(hwnd))
    {
        return TRUE;
    }

    RECT rect = {};
    if (!minimized
        && (!GetWindowRect(hwnd, &rect) || (rect.right - rect.left) < 24
            || (rect.bottom - rect.top) < 24))
    {
        return TRUE;
    }

    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);
    if (processId == 0 || processId == GetCurrentProcessId())
    {
        return TRUE;
    }

    WindowInfo info;
    info.handle = reinterpret_cast<WId>(hwnd);
    info.processId = processId;
    info.title = title;
    info.className = cls;
    info.minimized = minimized;
    info.executablePath = processPath(processId);
    if (!info.executablePath.isEmpty())
    {
        info.applicationName = fileDescription(info.executablePath);
        if (info.applicationName.isEmpty())
        {
            info.applicationName = QFileInfo(info.executablePath).completeBaseName();
        }
    }
    else
    {
        info.applicationName = title;
    }

    if (PathResolver::isExplorerClass(cls))
    {
        info.path = context->resolver->explorerPath(info.handle);
        if (info.path.isEmpty())
        {
            info.path = context->resolver->explorerPathFromAddressBar(info.handle);
        }
    }
    else if (PathResolver::isConsoleClass(cls))
    {
        info.path = context->resolver->consoleWorkingDirectory(processId);
    }

    info.identityKey =
        info.executablePath + QLatin1Char('|') + cls + QLatin1Char('|') + info.title;

    if (!info.executablePath.isEmpty())
    {
        auto iterator = context->iconCache->constFind(info.executablePath);
        if (iterator == context->iconCache->constEnd())
        {
            const QIcon icon = context->icons->icon(QFileInfo(info.executablePath));
            context->iconCache->insert(info.executablePath, icon);
            info.icon = icon;
        }
        else
        {
            info.icon = iterator.value();
        }
    }

    context->windows->append(info);
    return TRUE;
}
}

QVector<WindowInfo> WindowEnumerator::capture()
{
    PathResolver resolver;
    resolver.refreshExplorerPaths();

    QFileIconProvider iconProvider;
    QHash<QString, QIcon> iconCache;
    QVector<WindowInfo> windows;

    CaptureContext context;
    context.windows = &windows;
    context.resolver = &resolver;
    context.icons = &iconProvider;
    context.iconCache = &iconCache;

    EnumWindows(collectWindow, reinterpret_cast<LPARAM>(&context));

    return windows;
}







