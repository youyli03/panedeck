#pragma once

#include <QtGui/qwindowdefs.h>

#include <QHash>
#include <QString>

#include <windows.h>

/// Resolves Explorer folder paths (Shell COM) and console working directories
/// (PEB reads).
void paneDeckLog(const QString &message);

class PathResolver
{
public:
    void refreshExplorerPaths();
    QString explorerPath(WId handle) const;
    QString explorerPathFromAddressBar(WId handle) const;
    QString consoleWorkingDirectory(DWORD processId) const;

    static bool isExplorerClass(const QString &className);
    static bool isConsoleClass(const QString &className);

private:
    QHash<WId, QString> m_explorerPaths;
};

