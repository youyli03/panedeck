#pragma once

#include <QIcon>
#include <QString>

#include <windows.h>

/// One switchable top-level window shown on the board.
struct WindowInfo
{
    WId handle = 0;
    DWORD processId = 0;
    QString title;
    QString className;
    QString executablePath;
    QString applicationName;
    QString path;
    QString identityKey;
    QIcon icon;
    bool minimized = false;
};
