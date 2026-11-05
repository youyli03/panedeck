# PaneDeck

A Windows window dashboard. Groups open windows by application on a full-screen,
two-column board; double-click or press Enter to restore and raise a window.

- Platform: Windows 10 1809+ / Windows 11
- Stack: Qt 6.8 + MinGW 13.1 (C++20)
- Status: v0.2 prototype

## Features

- One group per application; no categories.
- Top tabs: `All` plus applications with more than 10 windows.
- Bordered group boxes; no scrollbar (overflow is clipped).
- Taskbar icons, English static UI, light theme.
- Explorer / cmd rows show full paths where readable; hover a row for the full path.
- Includes minimized windows; double-click or Enter restores and raises them.
- Drag to reorder (persisted); double-click or Enter activates; filtered and throttled auto-refresh.

## Install

Download the installer from Releases. It installs for the current user, adds a Start Menu shortcut (searchable from Windows Search), and offers an optional desktop shortcut.

## Build

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH=<Qt>/lib/cmake
cmake --build build
```

See [DESIGN.md](DESIGN.md).




