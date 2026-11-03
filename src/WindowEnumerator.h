#pragma once

#include "WindowInfo.h"

#include <QVector>

/// Captures the visible top-level windows.
class WindowEnumerator
{
public:
    QVector<WindowInfo> capture();
};
