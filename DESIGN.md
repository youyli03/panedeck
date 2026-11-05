# PaneDeck Design

> Version: v0.3 (2026-11-05 revision)
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
- Row height 24px; row content order: icon, title, then path/application text
  (adjacent, no separate right-hand column).
- Related Explorer rows may be preceded by a 20px section header (a small gray
  label with a hairline): `Case 08000000` or the shared parent folder.
- Grouping: by application (process), one group per application; no categories;
  groups sorted by window count descending.
- No scrollbar: overflow is clipped (a partially visible row indicates more
  content); wheel, keyboard, and drag still scroll.
- `docs/ui-mockup.png` shows the 50-window / 12-application layout.

## 3. Row content

- Icon (taskbar icon), title, then secondary text adjacent on the same line.
- File Explorer rows are path-paired: the title is the folder name and the gray
  text is the **containing folder** with a trailing separator (`build` +
  `F:\project\PaneDeck\`). The repeated leaf is not printed twice. When the
  title is not the path's last segment (drive roots, special folders), the full
  path is shown instead.
- Related Explorer rows are grouped under a labeled section:
  - `Case <id>` for a 6–8 digit case number shared by two or more windows;
  - the shared parent folder for two or more windows in the same non-root folder.
  - A case section shows each window's containing folder, so paired roots such
    as `F:\case\08000000` and `F:\other\08000000` read as two clear paths under
    one case.
- cmd / console: full working directory after the title.
- Other applications: title only; the application name is in the group header.
- Long paths are elided on the left (the immediate parent is kept); hovering a
  row or a section header shows the full path/label in a tooltip.

## 4. Interaction

- Drag: reorder rows inside one related-path section (persisted); moving across
  a section is not allowed because the section is derived from the folder path.
- Keyboard: the board starts with the first group focused; Up / Down / Home /
  End move the selection and scroll it into view; Enter activates. Navigation
  keys are routed to the board even when a tab button has focus, and choosing a
  tab returns focus to the board. Selection and keyboard focus are restored
  after a refresh. Only one row is highlighted at a time; selecting in another
  group clears the previous group.
- Double-click / Enter: restore the target window (`SW_RESTORE` when minimized)
  and bring it to the front, including occluded windows.
- Top options: `All` plus applications with more than 10 windows; selecting one
  filters to that application's group.
- Refresh: automatic. `SetWinEventHook` (CREATE / DESTROY / SHOW / HIDE /
  NAMECHANGE / FOREGROUND) with a 250 ms debounce triggers a full rescan; the
  selected window is tracked by its window handle and the focused application
  group is restored, so selection, focus, and scroll survive content changes,
  and the selected row is scrolled back into view. Refreshes are deferred while
  a mouse button is down so a drag is never interrupted by a rebuild.
  Refresh also runs on startup, panel show/restore, and system resume. No polling.
  Events from the dashboard's own process, non-top-level objects, untitled windows, and foreground-only changes are ignored; refreshes are throttled to 750 ms to prevent event storms from consuming CPU.
  F5 is a hidden fallback.

## 5. Visual

- Light theme with Fluent layering:
  - content background `#F3F3F3`; group boxes `#FFFFFF` with a 1px `#E2E2E2`
    border and a very light shadow; title bar/status bar `#F3F3F3`;
  - primary text `#1A1A1A`; paths `#6B6B6B` in Consolas 11px; counts `#8A8A8A`;
  - section headers `#8A8A8A` 11px with a hairline; a full-width divider closes
    a section before unsectioned rows;
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
  path 11px Consolas, counts 11px, status bar 11px.
- No scrollbar: overflow is clipped.

## 6. Grouping and ordering

- Group by application: one group per process/application; all windows of an
  application stay in the same group.
- Top options list an application only when it has more than 10 windows; all
  other applications appear only under `All`.
- Browsers (Edge / Chrome) are counted per top-level window; one browser window
  per row, titled by its active tab; tabs are not split into rows.
- Explorer path classification (`PathClassifier`):
  - Paths are canonicalized first (native separators, repeated separators
    collapsed, trailing separators dropped except on drive roots). `\\?\` /
    `\\.\` are stripped only when the remainder is a drive path; device and
    volume namespaces (`\\?\Volume{...}`, `\\.\COM1`) are kept verbatim and stay
    unsectioned.
  - A case id is a 6–8 digit segment. Date-looking ids (`YYYYMMDD`, `YYYYMM`,
    `YYMMDD`, `YYYYMDD`, `YYYYMMD`) are ignored unless the segment's parent is
    `case`/`cases`. A date-like id confirmed below `case`/`cases` also lets other
    windows with the same number join that case section. Non-ASCII digits never
    count. UNC paths start scanning after the server and share segments, so a
    numeric host or share name is not a case id.
  - Case sections take precedence over parent sections. A case section needs at
    least two windows with the same id. The exact id (including leading zeros)
    is part of the section key, so `123456` and `00123456` stay separate.
  - The id may be the leaf (`F:\case\08000000`) or a middle segment
    (`F:\other\08000000\logs`); both group together. If a path contains several
    numeric segments, a `case`/`cases` parent wins; otherwise a longer id wins
    (8 digits over 6), and for equal lengths the segment closest to the window's
    leaf wins, so a numeric ancestor such as a numeric user name does not steal
    the case.
  - Otherwise, two or more windows whose immediate parent folder is identical
    (case-insensitive) form a parent section, provided that parent is not a
    drive root (`F:`) or a UNC share root (`\\server\share`). Rows then show
    only their leaf names, because the header carries the folder once.
  - Rows without a resolved path (virtual folders, unreadable windows) stay
    plain: no section, title only.
  - Case sections sort numerically by id; parent sections by path; unsectioned
    rows last and by path. Within a case section rows sort by parent path, so
    different roots line up.
- Rows can be reordered by dragging inside one section; the order is persisted
  within that section.

## 7. Implementation

- Stack: Qt 6.8 + MinGW 13.1, C++20, Qt Widgets.
- Build: CMake (`CMakeLists.txt`); `tools/build.ps1` covers the local sandbox.
- Windows integration: `EnumWindows` for capture; Shell COM (`IShellWindows`)
  for Explorer folder paths; PEB reads for console working directories;
  `SetWinEventHook` with a 250 ms debounce for auto-refresh; `SetForegroundWindow`
  plus `AttachThreadInput` for activation.
- Explorer paths: Shell COM first, then the address-bar `ToolbarWindow32` fallback; console cwd via PEB (elevated consoles are not readable).
- Path classification is isolated in `src/PathClassifier.{h,cpp}`: pure functions for canonical paths, leaf/parent pairing, date-aware case ids, and section assignment. `MainWindow::buildGroups()` only maps the results onto `WindowInfo`.
- Unit tests in `tests/classification_test.cpp` cover drive/UNC roots, numeric
  server/share names, long-path prefixes, device/volume paths, mixed separators,
  dates, case anchors, zero-padded ids, case ids at the leaf vs in the middle,
  non-ASCII digits, Unicode names, unresolved paths, and section precedence. Enable with
  `-DPANEDECK_BUILD_TESTS=ON` and run `ctest`.
- Icons: Qt's `QFileIconProvider` on the process executable; group names come
  from version-info `FileDescription` when available (so they may be localized),
  falling back to the executable base name.
- The local toolchain lives under `tools/` and is ignored by Git.

## 8. Acceptance checklist

- [ ] Normal window with taskbar entry; minimize/close
- [ ] Full-screen dashboard; top options = All + applications with >10 windows
- [ ] Application groups with 1px borders
- [ ] Explorer rows pair the folder name with its containing folder; full path on hover
- [ ] Shared case numbers / parent folders form labeled sections; drive roots are not sections
- [ ] Date-looking folder names stay unsectioned unless anchored by case/cases
- [ ] UNC server/share names and device/volume paths are not misclassified
- [ ] Up/Down/Home/End selection works and stays visible; Enter activates
- [ ] Selection/focus survive refresh and window activation; arrows work after a tab click
- [ ] Only one row is highlighted across groups
- [ ] An auto-refresh does not interrupt a row drag
- [ ] cmd rows show full working directories
- [ ] Drag reorder persists across restarts (within a section)
- [ ] Double-click / Enter restores and raises (minimized/occluded)
- [ ] Automatic refresh on create/destroy/minimize/title change
- [ ] Light theme with taskbar icons
- [ ] `classification_test` passes

## 9. Open questions

1. Should the toolbar also offer sorting/settings beyond `All` and >10-window
   applications?














