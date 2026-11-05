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
    /// Explorer order key: the full folder path, so related folders list
    /// together. Empty for applications that are not path-based.
    QString sortPath;
    /// Secondary text painted after the title: the containing folder for
    /// Explorer windows, the working directory for consoles, empty otherwise.
    QString pathLabel;
    QString identityKey;
    QString sectionKey;
    /// Label for the related-path section header; empty for an unsectioned row.
    QString sectionLabel;
    QIcon icon;
    bool minimized = false;
};


