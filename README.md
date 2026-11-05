# PaneDeck

A Windows window dashboard. Groups open windows by application on a full-screen,
two-column board; double-click or press Enter to restore and raise a window.

- Platform: Windows 10 1809+ / Windows 11
- Stack: Qt 6.8 + MinGW 13.1 (C++20)
- Status: v0.3 prototype

## Features

- One group per application; no categories.
- Explorer rows are **path-paired**: the title is the folder name and the gray
  text is its containing folder (`build  F:\project\PaneDeck\`), so two windows
  with the same folder name stay distinguishable. Hover shows the full path.
- Related Explorer windows form a labeled section instead of floating apart:
  - a 6–8 digit case number repeated across windows (`Case 08000000`); the
    number may be the folder itself or appear in the middle of the path of a
    deeper window. Date-looking numbers (20240101, 202401, 240101, 2024011) are
    ignored unless the folder sits below `case`/`cases`, which also lets
    same-numbered sibling folders join the section;
  - otherwise an identical, non-root parent folder shared by two or more
    windows (`F:\project\PaneDeck\`, then rows `build`, `src`).
  - Drive roots and UNC share roots are never treated as related parents;
    UNC server/share names are never case ids, and device/volume paths
    (`\\?\Volume{...}`) stay unsectioned with their full path.
- Explorer rows and sections are ordered by folder path, so sibling folders sit
  together; sections come first, then unsectioned folders.
- Top tabs: `All` plus applications with more than 10 windows.
- Bordered group boxes; no scrollbar (overflow is clipped).
- Taskbar icons, English static UI, light theme.
- Explorer / cmd rows show paths where readable; hover a row for the full path.
- Includes minimized windows; double-click or Enter restores and raises them.
- Up / Down / Home / End move the selection and scroll it into view; Enter
  activates. Selection and keyboard focus survive automatic refreshes and window
  activation, and arrows still work after choosing a tab.
- Drag to reorder inside a section (persisted); refreshing is deferred while a
  mouse button is down, so a drag cannot be interrupted by the auto-refresh.

## Install

Download the installer from Releases. It installs for the current user, adds a Start Menu shortcut (searchable from Windows Search), and offers an optional desktop shortcut.

## Build

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH=<Qt>/lib/cmake
cmake --build build
```

## Test

The path classifier has unit tests covering drive/UNC roots, numeric server or
share names, long-path prefixes, device/volume paths, dates, case/cases anchors,
zero-padded case ids, mixed separators, Unicode and unresolved paths:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH=<Qt>/lib/cmake -DPANEDECK_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

See [DESIGN.md](DESIGN.md).











