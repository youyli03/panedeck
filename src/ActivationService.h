#pragma once

#include <QtGui/qwindowdefs.h>

/// Restores a minimized window and brings it to the foreground.
class ActivationService
{
public:
    static bool activate(WId handle);
};
