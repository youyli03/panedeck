# PaneDeck Design

> Version: v0.2 (2026-11-03 revision)  
> Platform: Windows 10 1809+ / Windows 11  
> Status: Draft; implementation in progress  
> One line: **a full-screen window dashboard, grouped by application, double-click to jump.**

---

## 0. Summary

PaneDeck groups all top-level windows by application and lays them out on a
full-screen, two-column board. Each row shows a window title and, for Explorer
and console windows, the full path. Drag to reorder; double-click to restore and
raise the target window.

## 1. Product form

- Normal desktop window: taskbar entry, minimize, close; launched normally.
- Full-screen dashboard: maximized; top option row; two-column group list.
- The dashboard stays open when a target window is activated.
- Language: all static UI strings are English; automatically fetched data
  (window titles, application names, process names, paths) keeps its original
  language.

## 2. Layout

- Desktop window structure: system title bar + toolbar + two-column content area
  + bottom status bar. Toolbar and status bar are fixed; content scrolls.
- Toolbar option row: `All` plus one option per application with more than 10
  windows. No manual refresh button.
- Content: two columns of groups. Each group is a white, 1px-bordered rounded
  box: header (application icon + name + window count) and rows.
- Row height 24px; row content order: icon, title, path/application (adjacent,
  no separate right-hand column).
- Grouping: by application (process), one group per application; no categories;
  groups sorted by window count descending.
- No scrollbar: overflow is clipped (a partially visible row indicates more
  content); wheel, keyboard, and drag still scroll.
- `docs/ui-mockup.png` shows the 50-window / 12-application layout.

## 3. Row content

- Icon (taskbar icon), title, then (path/application) adjacent on the same line.
- File Explorer: full folder path in small gray text after the title.
- cmd / console: full working directory after the title.
- Other applications: title only; the application name is in the group header.
- Long paths are elided in the middle; hovering a row shows the full path in a tooltip.

## 4. Interaction

- Drag: reorder windows inside a group (persisted); groups can also be reordered.
- Double-click / Enter: restore the target window (`SW_RESTORE` when minimized)
  and bring it to the front, including occluded windows.
- Top options: `All` plus applications with more than 10 windows; selecting one
  filters to that application's group.
- Refresh: automatic. `SetWinEventHook` (CREATE / DESTROY / SHOW / HIDE /
  NAMECHANGE / FOREGROUND) with a 250 ms debounce triggers a full rescan; items
  are merged by identity so order, selection, and scroll position are preserved.
  Refresh also runs on startup, panel show/restore, and system resume. No polling.
  Events from the dashboard's own process, non-top-level objects, untitled windows, and foreground-only changes are ignored; refreshes are throttled to 750 ms to prevent event storms from consuming CPU.
  F5 is a hidden fallback.

## 5. Visual

- Light theme with Fluent layering:
  - content background `#F3F3F3`; group boxes `#FFFFFF` with a 1px `#E2E2E2`
    border and a very light shadow; title bar/status bar `#F3F3F3`;
  - primary text `#1A1A1A`; paths `#6B6B6B` in Consolas 10px; counts `#8A8A8A`;
  - selected row `#EDF1F5` with a 3px amber `#B26B1F` bar on the left.
- The top-left title shows **PaneDeck** only, in English.
- Window corners follow the OS: Windows 11 rounds them automatically, Windows 10
  keeps them square. No custom borderless window corners. Group boxes use 8px
  radius.
- Every content surface has a 1px border; layout is not communicated by
  whitespace alone.
- Icons: the same icons as the taskbar (extracted from the window/process at
  runtime); a letter placeholder is used only when no icon is available.
- Fonts: Segoe UI / Microsoft YaHei UI; group header 13px/600, row title 12.5px,
  path 10px Consolas, counts 11px, status bar 11px.
- No scrollbar: overflow is clipped.

## 6. Grouping and ordering

- Group by application: one group per process/application; all windows of an
  application stay in the same group.
- Top options list an application only when it has more than 10 windows; all
  other applications appear only under `All`.
- Browsers (Edge / Chrome) are counted per top-level window; one browser window
  per row, titled by its active tab; tabs are not split into rows.
- Groups and rows can be reordered by dragging; order is persisted.

## 7. Implementation

- Stack: Qt 6.8 + MinGW 13.1, C++20, Qt Widgets.
- Build: CMake (`CMakeLists.txt`); `tools/build.ps1` covers the local sandbox.
- Windows integration: `EnumWindows` for capture; Shell COM (`IShellWindows`)
  for Explorer folder paths; PEB reads for console working directories;
  `SetWinEventHook` with a 250 ms debounce for auto-refresh; `SetForegroundWindow`
  plus `AttachThreadInput` for activation.
- Explorer paths: Shell COM first, then the address-bar `ToolbarWindow32` fallback; console cwd via PEB (elevated consoles are not readable).
- Icons: Qt's `QFileIconProvider` on the process executable; group names come
  from version-info `FileDescription` when available (so they may be localized),
  falling back to the executable base name.
- The local toolchain lives under `tools/` and is ignored by Git.

## 8. Acceptance checklist

- [ ] Normal window with taskbar entry; minimize/close
- [ ] Full-screen dashboard; top options = All + applications with >10 windows
- [ ] Application groups with 1px borders
- [ ] Explorer rows show full folder paths
- [ ] cmd rows show full working directories
- [ ] Drag reorder persists across restarts
- [ ] Double-click / Enter restores and raises (minimized/occluded)
- [ ] Automatic refresh on create/destroy/minimize/title change
- [ ] Light theme with taskbar icons

## 9. Open questions

1. Should the toolbar also offer sorting/settings beyond `All` and >10-window
   applications?








