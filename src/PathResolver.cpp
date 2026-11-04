#include "PathResolver.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>
#include <QString>

#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <exdisp.h>
#include <shldisp.h>
#include <oleauto.h>

#include <QCoreApplication>
#include <QDateTime>

#include <cstdio>
#include <cstring>
#include <iterator>
#include <string>

void paneDeckLog(const QString &message)
{
    const QString directory = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QDir().mkpath(directory);
    QFile file(directory + QStringLiteral("/diagnostics.log"));
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    {
        QTextStream stream(&file);
        stream << QDateTime::currentDateTime().toString(Qt::ISODate) << ' ' << message << '\n';
    }
}

namespace
{
constexpr DWORD kProcessQueryInformation = 0x0400;
constexpr DWORD kProcessQueryLimitedInformation = 0x1000;
constexpr DWORD kProcessVmRead = 0x0010;

// GUIDs are defined locally so MinGW does not need the uuid import library.
const CLSID kShellWindowsClsid =
    {0x9BA05972, 0xF6A8, 0x11CF, {0xA4, 0x42, 0x00, 0xA0, 0xC9, 0x0A, 0x8F, 0x39}};
const IID kShellWindowsIid =
    {0x85CB6900, 0x4D95, 0x11CF, {0x96, 0x0C, 0x00, 0x80, 0xC7, 0xF4, 0xEE, 0x85}};
const IID kWebBrowser2Iid =
    {0xD30C1661, 0xCDAF, 0x11D0, {0x8A, 0x3E, 0x00, 0xC0, 0x4F, 0xC9, 0xE2, 0x6E}};

using NtQueryInformationProcessFn = LONG(NTAPI *)(HANDLE, ULONG, PVOID, ULONG, PULONG);

NtQueryInformationProcessFn ntQueryInformationProcess()
{
    static auto function = reinterpret_cast<NtQueryInformationProcessFn>(
        GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtQueryInformationProcess"));
    return function;
}

bool readPointer(HANDLE process, void *address, int pointerSize, quintptr *value)
{
    if (pointerSize == 8)
    {
        ULONG_PTR buffer = 0;
        if (!ReadProcessMemory(process, address, &buffer, sizeof(buffer), nullptr))
        {
            return false;
        }
        *value = static_cast<quintptr>(buffer);
        return true;
    }

    ULONG buffer = 0;
    if (!ReadProcessMemory(process, address, &buffer, sizeof(buffer), nullptr))
    {
        return false;
    }
    *value = static_cast<quintptr>(buffer);
    return true;
}

QString readUnicodeString(HANDLE process, void *address, int pointerSize)
{
    USHORT length = 0;
    if (!ReadProcessMemory(process, address, &length, sizeof(length), nullptr) || length == 0)
    {
        return {};
    }

    if (length > 0x8000)
    {
        return {};
    }

    quintptr buffer = 0;
    const int bufferOffset = pointerSize == 8 ? 8 : 4;
    if (!readPointer(process, static_cast<char *>(address) + bufferOffset, pointerSize, &buffer)
        || buffer == 0)
    {
        return {};
    }

    QByteArray bytes(length, Qt::Uninitialized);
    if (!ReadProcessMemory(process, reinterpret_cast<void *>(buffer), bytes.data(), length, nullptr))
    {
        return {};
    }

    return QString::fromUtf16(reinterpret_cast<const char16_t *>(bytes.constData()),
                              length / static_cast<int>(sizeof(char16_t)));
}

QString urlToLocalPath(const QString &url)
{
    if (!url.startsWith(QStringLiteral("file:"), Qt::CaseInsensitive))
    {
        return {};
    }

    wchar_t buffer[MAX_PATH * 4] = {};
    DWORD length = static_cast<DWORD>(std::size(buffer));
    const std::wstring wide = url.toStdWString();
    if (SUCCEEDED(PathCreateFromUrlW(wide.c_str(), buffer, &length, 0)))
    {
        return QString::fromWCharArray(buffer);
    }

    return {};
}
}

void PathResolver::refreshExplorerPaths()
{
    m_explorerPaths.clear();

    IShellWindows *shellWindows = nullptr;
    const HRESULT createResult = CoCreateInstance(kShellWindowsClsid, nullptr, CLSCTX_ALL,
                                                  kShellWindowsIid,
                                                  reinterpret_cast<void **>(&shellWindows));
    if (FAILED(createResult) || shellWindows == nullptr)
    {
        return;
    }

    long count = 0;
    if (SUCCEEDED(shellWindows->get_Count(&count)))
    {
        for (long index = 0; index < count; ++index)
        {
            VARIANT variant;
            VariantInit(&variant);
            variant.vt = VT_I4;
            variant.lVal = index;

            IDispatch *dispatch = nullptr;
            if (FAILED(shellWindows->Item(variant, &dispatch)) || dispatch == nullptr)
            {
                continue;
            }

            IWebBrowser2 *browser = nullptr;
            if (SUCCEEDED(dispatch->QueryInterface(kWebBrowser2Iid,
                                                   reinterpret_cast<void **>(&browser)))
                && browser != nullptr)
            {
                SHANDLE_PTR handleValue = 0;
                if (SUCCEEDED(browser->get_HWND(&handleValue)) && handleValue != 0)
                {
                    BSTR locationUrl = nullptr;
                    if (SUCCEEDED(browser->get_LocationURL(&locationUrl)) && locationUrl != nullptr)
                    {
                        const QString path =
                            urlToLocalPath(QString::fromWCharArray(locationUrl,
                                                                  SysStringLen(locationUrl)));
                        if (!path.isEmpty())
                        {
                            m_explorerPaths.insert(static_cast<WId>(handleValue), path);
                        }
                        SysFreeString(locationUrl);
                    }
                }

                browser->Release();
            }

            dispatch->Release();
        }
    }

    shellWindows->Release();
}

QString PathResolver::explorerPath(WId handle) const
{
    return m_explorerPaths.value(handle);
}

QString PathResolver::explorerPathFromAddressBar(WId handle) const
{
    struct Context
    {
        QString path;
    } context;

    EnumChildWindows(
        reinterpret_cast<HWND>(handle),
        [](HWND child, LPARAM parameter) -> BOOL
        {
            auto *context = reinterpret_cast<Context *>(parameter);
            wchar_t classNameBuffer[128] = {};
            if (GetClassNameW(child, classNameBuffer, static_cast<int>(std::size(classNameBuffer)))
                <= 0)
            {
                return TRUE;
            }

            const bool isEdit = _wcsicmp(classNameBuffer, L"Edit") == 0;
            const bool isToolbar = _wcsicmp(classNameBuffer, L"ToolbarWindow32") == 0;
            if (!isEdit && !isToolbar)
            {
                return TRUE;
            }

            wchar_t textBuffer[2048] = {};
            const int length = GetWindowTextW(child, textBuffer,
                                              static_cast<int>(std::size(textBuffer)));
            if (length <= 0)
            {
                return TRUE;
            }

            const QString text = QString::fromWCharArray(textBuffer, length).trimmed();
            QString candidate = text;

            // The address bar text is prefixed with a localized "Address: ".
            const int driveIndex = candidate.indexOf(QStringLiteral(":\\"));
            const int uncIndex = candidate.indexOf(QStringLiteral("\\\\"));
            if (driveIndex >= 2 && candidate.at(driveIndex - 1).isLetter())
            {
                candidate = candidate.mid(driveIndex - 1).trimmed();
            }
            else if (uncIndex >= 0)
            {
                candidate = candidate.mid(uncIndex).trimmed();
            }
            else
            {
                return TRUE;
            }

            if (candidate.contains(QStringLiteral(":\\")) || candidate.startsWith(QStringLiteral("\\\\")))
            {
                context->path = candidate;
                return FALSE;
            }

            return TRUE;
        },
        reinterpret_cast<LPARAM>(&context));

    return context.path;
}
QString PathResolver::consoleWorkingDirectory(DWORD processId) const
{
    HANDLE process = OpenProcess(kProcessQueryInformation | kProcessVmRead, FALSE, processId);
    if (process == nullptr)
    {
        process = OpenProcess(kProcessQueryLimitedInformation | kProcessVmRead, FALSE, processId);
    }
    if (process == nullptr)
    {
        return {};
    }

    QString result;
    const auto ntQuery = ntQueryInformationProcess();
    if (ntQuery == nullptr)
    {
        CloseHandle(process);
        return {};
    }

    PVOID peb = nullptr;
    int pointerSize = 8;
    quintptr wow64Peb = 0;
    if (ntQuery(process, 26 /* ProcessWow64Information */, &wow64Peb, sizeof(wow64Peb), nullptr) == 0
        && wow64Peb != 0)
    {
        peb = reinterpret_cast<PVOID>(wow64Peb);
        pointerSize = 4;
    }
    else
    {
        struct ProcessBasicInformation
        {
            PVOID reserved1;
            PVOID pebBaseAddress;
            PVOID reserved2[2];
            ULONG_PTR uniqueProcessId;
            PVOID reserved3;
        } basic = {};

        if (ntQuery(process, 0 /* ProcessBasicInformation */, &basic, sizeof(basic), nullptr) == 0)
        {
            peb = basic.pebBaseAddress;
        }
    }

    if (peb != nullptr)
    {
        const int processParametersOffset = pointerSize == 8 ? 0x20 : 0x10;
        const int currentDirectoryOffset = pointerSize == 8 ? 0x38 : 0x24;

        quintptr parameters = 0;
        if (readPointer(process, static_cast<char *>(peb) + processParametersOffset, pointerSize,
                        &parameters)
            && parameters != 0)
        {
            result = readUnicodeString(process,
                                       reinterpret_cast<void *>(parameters + currentDirectoryOffset),
                                       pointerSize);
        }
    }

    CloseHandle(process);
    return result;
}

bool PathResolver::isExplorerClass(const QString &className)
{
    return className == QStringLiteral("CabinetWClass")
        || className == QStringLiteral("ExploreWClass");
}

bool PathResolver::isConsoleClass(const QString &className)
{
    return className == QStringLiteral("ConsoleWindowClass");
}










